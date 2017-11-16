# POSIX named semaphores example
Roman Smirnov      
14/11/2017

## Q1


Solution code was written in the same file due to it being very short and non-reusable.

The solution utilizes named semaphores (NS) with some thin wrapper functions to handle errors.

NS were chosen because unnamed semaphores are POSIX-optional and are not currently supported by macOS (the OS used to write the code).

The NS is create and opened by the parent process, and immediately unlinked (to avoid NS persisting between runs, it remains open), the open NS is then passed to child process (to avoid having each child read from the filesystem).
The update function get the binary NS from each child process and locks it until it's done.

Advantages: more portable.
Disadvantages: more complicated, less performant, more code, more pitfalls (due to persistence), further pitfalls due to binary semaphores not being the same as mutexs.

## Q2
Syscalls cause the kernel to take over, perform the operation, and hand back the results to the user-space caller. Function calls occur within programs in user-space.

## Q3
The kernel provides an interface to the hardware and essential services.
The disadvantage of this model is that it's slower because of the overhead of having to manage clients.

## Q4
Function calls happen in user-space inside the same process.
Signals are a method of communication between privileged processes and between the kernel/processes.
Interrupts are a method of communication between the CPU and the kernel.

## Q5.A
Consider the code below.
There could be a situation where incrementing and decrementing is done in close
temporal proximity and an element will be missed because the operations are not atomic.
For example: avail = avail + 1 does a read operation (get the value of avail) and then a
write operation (write the new value of avail), if the value of avail is decremented between
these two operations we end up missing queue elements.

```

 /* maximum number of elements in queue*/
#define MAX 100
queue_element_type queue[MAX];

/* initially the queue is empty - producer can put MAX elements into it */
int avail = MAX;

void producer() {
  queue_element_type queue_elm;
  int head = 0; /* track position in queue circle */

  while(1){ /* begin loop */
    if (avail > 0){ /* we can put items in queue */
      queue_elm = produce_stuff() /* produce a new queue element */
      queue[head % MAX] = queue_elm; /* insert new elmement */
      head = head + 1;
      avail = avail - 1;    // can produce one less
    }
  } /* end loop */
}

void consumer() {
  type_queue_element queue_elm;
  int tail = 0; /* tracks position in queue circle */
  while(1) { /* begin loop */
    if (avail < MAX) { /* we can collect items from queue */
      /* get item from queue */
      queue_elm = queue[head % tail];
      tail = tail + 1;
      avail = avail + 1;    // can produce one more
      /* consumer does something with the element it got */
      do_stuff(queue_elm);
    }
  } /* end loop */
}

int main()
{
  /* create and run consumer and producer threads */
}

```


## Q5.B+C
We use a mutex (in this case, binary semaphore is fine ) and 2 semaphores.
The mutex ensures only 1 thread modifies the queque at a time.
The semaphores allow each thread to be inactive while it's not needed.


```

/* maximum and min number of elements in queue*/
#define MAX 100
#define MIN 0
queue_element_type queue[MAX];

/* lock to ensure queue is accessed only by one thread at a time */
type_mutex mutex;

/* each semaphore initiated with corresponding value in main */
type_counting_semaphore sem_p;
type_counting_semaphore sem_c;


void producer() {
  queue_element_type queue_elm;
  int head = 0; /* track position in queue circle */

  while(1){ /* begin loop */

    /* produce a new queue element */
    queue_elm = produce_stuff();

    /* sleep until there is space in the queue */
    sem_wait(sem_p);

    /* get exclusive access to queue */
    mutex_lock(mutex);

    /* put item into queue */
    queue[head % MAX] = queue_elm; /* insert new elmement */
    head = head + 1;
    sem_signal(sem_c);   /* one more to consume */
    mutex_unlock(mutex);

  } /* end loop */
}

void consumer() {
  type_queue_element queue_elm;
  int tail = 0; /* tracks position in queue circle */

  while(1) { /* begin loop */

     /* sleep until there are items in the queue */
      sem_wait(sem_c);

      /* get exclusive access to queue */
      mutex_lock(mutex);

    /* get item from queue */
      queue_elm = queue[tail % MAX];

      tail = tail + 1;

      sem_signal(sem_p);
      mutex_unlock(mutex);

      /* consumer does something with the element it got */
      do_stuff(queue_elm);

  } /* end loop */
}


int main()
{
   /* init producer semaphore */
   sem_p = semaphore_init(MAX);
  /* init consumer semaphore */
   sem_c = semaphore_init(MIN);
   /* init mutex */
   mutex_init(mutex);
  /* create and run consumer and producer threads */
}


```

The End
