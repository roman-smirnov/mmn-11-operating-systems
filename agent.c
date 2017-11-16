/**********************************************************************************************
 File: agent.c

 
 Description:         Here are all the function needed to
                      initiate the work of the agents.


**********************************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "agent.h"
#include <semaphore.h>  // POSIX semaphores
#include <fcntl.h>  // required for linux compat

/**********************************************************************************************/
/*                                prototypes                                                  */

int init(int);

int check_point(int);

int sum_up(char *);

void fatal_error(char *, char *);

void my_exit(int);

int go_agent(char *, sem_t *);

int gen_rand(int);

int update(int, char *, char *, sem_t *);

int write_to_log(int, char *);

sem_t *open_semaphore();

void close_semaphore(sem_t *sem);

void unlink_semaphore();




/**********************************************************************************************/
/*                            Global Array - All Agents' names                                */
char agents[MAX_AGENTS][NAMES_LEN] = AGENTS;


/**********************************************************************************************/
int main(int ac, char **av) /* Reset all files,Dispatch all agents and control actions */
{
    int i = 0,
            status,
            p,                 /* number of the started agents */
            num_agents = 1;    /* default */

    int process[MAX_AGENTS];


    if (ac == 2)                            /* Check arguments */
        i = atoi(av[1]);  // convert string to int
    else if (ac != 1)
        fatal_error("USAGE: <name_of_the_program> [number_of_agents]\n", NULL);


    /* initiate an account file with a start sum */
    num_agents = init(i);


    /* create or open a named binary semaphore */
    sem_t *sem = open_semaphore();

    /* avoid semaphore persistence - i.e delete from filesystem, but keep open */
    unlink_semaphore();

    /* Father distributes work to sons (agents).
       Agents perform independent operations.
       Every agent updates the account file after each
       operation and logs them in a personal log file */
    for (i = 0, p = 0; i < num_agents; i++, p++)
        if ((process[p] = go_agent(agents[p], sem)) == ERROR) {
            fprintf(stderr, "Agent %s Can't work\n", agents[p]);
            perror("While Fork ");
            --p;
        }
    i = p;

    /* The father waits for agents to return successfully */
    while (p >= 1) {
        if (wait(&status) < 0)
            fatal_error("wait failed", "wait:");

        if (!(WIFEXITED(status) && (WEXITSTATUS(status) == SUCCESS)))  /* kill all running agents */
        {
            fprintf(stderr, "Agent failed. Killing all rnning agents.\n");
            while (p >= 0)
                if (kill(process[p--], SIGINT) < 0) perror("");
            my_exit(1);
        }
        p--;
    }

    /* close the semaphore - not actually needed, just a reminder */
    close_semaphore(sem);

    /* check the consistency (log files vs.
       the amount written in the account file. */
    if (check_point(i) == ERROR)
        fatal_error("Can't control Account", ACCOUNT);


    return 0;
}


/**********************************************************************************************/
/*  initiate the account file with the starting sum 
 *  return the number of agents to be crated in the range [1, MAX_AGENTS]
 *
 */
int init(int num_agents)   /* Initiate auxiliary files,returns number agents */
{
    FILE *fp;
    int i;

    if (num_agents < 1 || num_agents > MAX_AGENTS) {
        printf("%d agent(s) defaults to 1\n", num_agents);
        num_agents = 1;
    }

    printf("Creating %d agent(s). \n", num_agents);

    // delete old files; exit if fails
    for (i = 0; i < num_agents; i++)
        if ((unlink(agents[i]) < 0) && (errno != ENOENT))
            fatal_error("Deleting agents log", "");

    if ((fp = fopen(ACCOUNT, "w")) == NULL)
        fatal_error("Creating ACCOUNT file", "");
    fprintf(fp, "%d\n", START_SUM);
    fclose(fp);

    return (num_agents);
}


/**********************************************************************************************/
int check_point(int num_agents) /* Are agents' actions consistent with account? */
{
    int i, control_sum = 0, sum = START_SUM;

    printf("\n\n ----- Controler ----- \n");


    for (i = 0; i < num_agents; ++i)
        control_sum = control_sum + sum_up(agents[i]);

    control_sum = sum + control_sum;

    if ((sum = update(0, ACCOUNT, NULL, NULL)) == ERROR)
        return (ERROR);
    if (sum != control_sum) {
        fprintf(stderr, "\n Thieves in the system!\n");
        fprintf(stderr, "Controler: %d, Account: %d \n", control_sum, sum);
        my_exit(1);
    }
    printf("\nOperations O.K. Controler: %d, Acount: %d\n", control_sum, sum);

    return SUCCESS;
}


/**********************************************************************************************/
int sum_up(char *log_file)    /* Returns the sum of log_file */
{
    FILE *fp;
    int sum = 0, tmp_sum;

    if ((fp = fopen(log_file, "r+")) == NULL) {
        perror(log_file);
        printf("\nCan't Control %s agent!\n", log_file);
        return (0);
    }
    while (fscanf(fp, "%d", &tmp_sum) > 0)
        sum = sum + tmp_sum;
    fclose(fp);

    printf(" Sum of %s = %d\n", log_file, sum);
    return (sum);
}


/**********************************************************************************************/
void fatal_error(char *err_msg, char *perror_arg)    /* Prints Error messages and exit(ERROR) */
{

    fprintf(stderr, "\nERROR: %s. ", err_msg);
    if (perror_arg != NULL)
        perror(perror_arg);
    my_exit(ERROR);


}


/**********************************************************************************************/

void my_exit(int status) {


    exit(status);
}

/**********************************************************************************************/

int go_agent(char *agent_name, sem_t *sem)      /* sends agent to work */
{
    int n,
            i,
            amount,
            pid,
            retry = 0;

    /* fork the process */
    while (((pid = fork()) < 0) && (++retry < RETRY)) // fork the process
        sleep(SLEEP);

    if (pid == 0) { //it's a child  process

        /* do all the agent operations */
        for (i = 0; i < OPERATIONS; ++i) {

            /* generate a random amount to deposit/withdraw*/
            amount = gen_rand(MAX_OPER);


            /* update the mutual account */
            if (update(amount, ACCOUNT, agent_name, sem) == ERROR)
                fatal_error("Can't Update Account File", ACCOUNT);

            /* update the personal log */
            if (write_to_log(amount, agent_name) == ERROR) {
                // rollback if there was an error writing to personal log
                if (update(-amount, ACCOUNT, NULL, sem) == ERROR)
                    fprintf(stderr, "ERROR: Rollback failed. Agent %s\n", agent_name);
                fatal_error("Can't log operations", agent_name);
            }

            /* sleep for some random time */
            if ((n = rand() % MAX_SLEEP) != 0)
                sleep(n);
        }
        printf(" Agent %s : Succefull!\n", agent_name);
        exit(0);  /* Son normally exits here! */
    }

    return ((pid < 0) ? ERROR : pid);
}


/**********************************************************************************************/
int gen_rand(int range)  /* Generates random numbers between -rang and +range */
/* exept 0, if range = 0 - init generation */
{
    int n, sign;
    static int init = 0;

    if (init++ == 0)
        srand(time(NULL) % getpid());

    while ((n = rand()) == 0);
    sign = (n / 1111) % 2;
    n = ((n / 1117) % range) + 1;

    if (sign == 0)
        return (n);
    else return (0 - n);
}


/**********************************************************************************************/
int update(int amount, char *account, char *name, sem_t *sem)  /* Updates account using amount. Returns new sum */
{

    FILE *fp;
    int n;

    char buf[BUF_LENGTH];

    memset (buf, '\0', BUF_LENGTH);

    if(sem != NULL)
        sem_wait(sem);  // acquire lock

    if (!(fp = fopen(account, "r")))
        return (ERROR);


    fscanf(fp, "%s", buf);
    fclose(fp);

    if (amount != 0) printf("%s", ".");

    if (!(fp = fopen(account, "w")))
        return (ERROR);

    fprintf(fp, "%d", n = atoi(buf) + amount);
    fclose(fp);

    if(sem != NULL)
        sem_post(sem); // release lock

    return (n + amount);                   /* New sum in account */
}


/**********************************************************************************************/
int write_to_log(int amount, char *log_file)               /* Appends amount to log_file */
{
    FILE *fp;

    if ((fp = fopen(log_file, "a")) == NULL)
        return (ERROR);

    fprintf(fp, "%d\n", amount);
    fclose(fp);
    return (SUCCESS);
}


/**********************************************************************************************/
/*                                semaphore handling                                         */
/**********************************************************************************************/

// TODO: check makefile actually works
// TODO: add required header for linux compat

/**
 * create a semaphore if it doesn't exist yet or return the existing one if it already exists
 * @return pointer to semaphore
 */
sem_t *open_semaphore() {
    sem_t *sem; /* semaphores are passed by address */
    if ((sem = sem_open(SEM_NAME, O_CREAT, SEM_PERM, SEM_INIT_VAL)) == SEM_FAILED) {
        fatal_error("couldn't open semaphore", "sem_open");
    }
    return sem;
}

/**
 * close the semaphore
 * @param sem
 */
void close_semaphore(sem_t *sem){
    if (sem_close(sem) == -1) {
        fatal_error("couldn't close semaphore", "sem_close");
    }
}

/**
 * unlink the named semaphore
 */
void unlink_semaphore() {
    if (sem_unlink(SEM_NAME) == -1) {
        fatal_error("couldn't remove semaphore", "sem_unlink");
    }
}

/**********************************************EOF*********************************************/
