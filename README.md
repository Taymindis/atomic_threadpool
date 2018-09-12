# atomic_threadpool
it is c/c++ library, it's a smallest library that provides a lock-free thread pool sharing on multithreading, it design for scalability


## For Linux OS build

## Installation

```bash
mkdir build

cd build

cmake ..

make

sudo make install

```



## Uninstallation

```bash
cd build

sudo make uninstall

```


## Example to run
```c

#include <stdio.h>
#include <pthread.h>
#include "at_thpool.h"


#define TASK_SIZE 200

void t1(void *arg);
void t2(void *arg);
void t3(void *arg);
void t4(void *arg);

void t1(void *arg) {
	printf("t1 is running on thread #%u \n", (int)pthread_self());
}

void t2(void *arg) {
	printf("t2 is running on thread #%u\n", (int)pthread_self());
}

void t3(void *arg) {
	printf("t3 is running on thread #%u\n", (int)pthread_self());
}

void t4(void *arg) {
	printf("t4 is running on thread #%u\n", (int)pthread_self());
}

int main(void) {
	int nthreads = 8;//sysconf(_SC_NPROCESSORS_ONLN); // Linux

	printf("Share thread pool with %d threads with at lease totalthroughput * nthreads task size\n", nthreads);
	at_thpool_t *thpool = at_thpool_create(nthreads);

	printf("assigned %d tasks between %d threads\n", TASK_SIZE, nthreads);
	int i;
	for (i = 0; i < TASK_SIZE; i++) {
		at_thpool_newtask(thpool, t1, NULL);
		at_thpool_newtask(thpool, t2, NULL);
		at_thpool_newtask(thpool, t3, NULL);
		at_thpool_newtask(thpool, t4, NULL);
	}

#if defined _WIN32 || _WIN64
	Sleep(1000);
#else
	usleep(1000 * 1000);
#endif

	puts("shutdown thread pool");
	at_thpool_gracefully_shutdown(thpool);

	return 0;
}

```


### How to wait for task to finish on main thread

AtomicThreadPool design for lock free, it does not wait for anyone, to wait for specified task to finished, client should pass the value to indicate it has done.

For Example for wait task: 

```c

#include <stdio.h>
#include <pthread.h>
#include "at_thpool.h"

typedef struct myagent_s {
	int has_task_done;
	void *my_value;
} my_agent;

void t1(void *arg);

void t1(void *arg) {
	my_agent *agent = (my_agent*) arg;

	printf("t1 is running on thread #%u \n", (int)pthread_self());

	/** Doing heavy task **/

	agent->has_task_done = 1;

}

int main(void) {
	int nthreads = 8;//sysconf(_SC_NPROCESSORS_ONLN); // Linux

	printf("Share thread pool with %d threads with at lease totalthroughput * nthreads task size\n", nthreads);
	at_thpool_t *thpool = at_thpool_create(nthreads);

	int i;
	my_agent agent;
	agent.has_task_done = 0

	at_thpool_newtask(thpool, t1, &agent);
while (!agent.has_task_done) {
#if defined _WIN32 || _WIN64
	Sleep(1);
#else
	usleep(1000);
#endif
}

	puts("shutdown thread pool");
	at_thpool_gracefully_shutdown(thpool);

	return 0;
}


```


## For manual build
gcc -std=c11 -I./ -Ilfqueue/ at_thpool.c lfqueue/lfqueue.c threadpool_example.c -pthread


## for Windows os build

### Recommend to use VS2017 to build

#### include the sources file at_thpool.c at_thpool.h lfqueue.c lfqueue.h into VS2017 project solution.

Alternatively, 

#### Download the Dev-C++ IDE - https://sourceforge.net/projects/orwelldevcpp/



#### You can use any IDE/build tools as you wish, just include the source files to your project




