
#include <stdio.h>
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#include <pthread.h>
#elif defined _WIN32 || _WIN64
#include <windows.h>
#include <Windows.h>
#endif
#include "at_thpool.h"


#define TASK_SIZE 200

void t1(void *arg);
void t2(void *arg);
void t3(void *arg);
void t4(void *arg);

void t1(void *arg) {
	printf("t1 is running on thread \n");
}

void t2(void *arg) {
	printf("t2 is running on thread \n");
}

void t3(void *arg) {
	printf("t3 is running on thread \n");
}

void t4(void *arg) {
	printf("t4 is running on thread \n");
}

int main(void) {
	int nthreads = 8;//sysconf(_SC_NPROCESSORS_ONLN); // Linux

	printf("Share thread pool with %d threads with at lease totalthroughput * nthreads task size\n", nthreads);
	at_thpool_t *thpool = at_thpool_create(nthreads, nthreads * TASK_SIZE);

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