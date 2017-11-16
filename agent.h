/* ________________________________________________________________________

    Header File: agent.h
      of the program AGENTS.C.  part 1 of 3.

      Related Files:           1. agent.h   -  Global Constants.
                               2. father.c  -  main program.
                               3. agent.c   -  Agent actions.
                               4. agent.doc -  Doc file.

      Description:         Here are all the Constants needed to
                           initiate the work of the agents program.

      Note:                You may change some for expermintal use.
__________________________________________________________________________ */

#ifndef __AGENT_H
#define __AGENT_H

// parameters for creating agents semaphore
#define SEM_PERM    0644  // permission given to semaphore
#define SEM_INIT_VAL    1 // initial value of semaphore when created
#define SEM_NAME    "/semaphore"  // name of named semaphore

#define OPERATIONS  5       /* Number of Operation for each Agent      */
                             /* Change it to 30 to check your solution. */

#define MAX_OPER    10       /* Maximum absolute order of one operation */

#define START_SUM  100
#define ACCOUNT  "account"   /* Name of Account File */

#define ERROR    999999 /* smth very unlogical for sum to get upto this limit*/
#define SUCCESS     0

#define BUF_LENGTH  64       /* buffer size used by read */
#define NAMES_LEN   2        /* Agent's Name length + 1 */
#define MAX_AGENTS  8        /* Maximum number of agents */
#define AGENTS {"a","b","c","d","e","f","g","h"} /*Depends upon MAX_AGENTS!*/

#define RETRY      20        /* Number of retries for system calls */
#define MAX_SLEEP  5         /* Maximum seconds of rest for agent + 1 */
#define SLEEP      5         /* Sleeping time before retry of system call */


#endif

