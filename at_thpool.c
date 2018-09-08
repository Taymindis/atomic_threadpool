/*
* BSD 2-Clause License
* 
* Copyright (c) 2018, Taymindis Woon
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* 
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#include <pthread.h>
#include <sched.h>
#elif defined _WIN32 || _WIN64
#include <windows.h>
#include <Windows.h>
#endif

#include "lfqueue.h"
#include "at_thpool.h"

/**
* Over max thread will affect the system scalability, it might not scale well
*/
#define MAX_THREADS 64
#define MAX_LFQUEUE 65536

#define AT_THPOOL_ERROR perror
#define DEF_SPIN_LOCK_CYCLE 2048
#define AT_THPOOL_MALLOC malloc
#define AT_THPOOL_FREE free

#if defined _WIN32 
#define AT_THPOOL_INC(v) InterlockedExchangeAddNoFence(v, 1)
#define AT_THPOOL_DEC(v) InterlockedExchangeAddNoFence(v, -1)
#define AT_THPOOL_SHEDYIELD SwitchToThread
#elif defined _WIN64
#define AT_THPOOL_INC(v) InterlockedExchangeAddNoFence64(v, 1)
#define AT_THPOOL_DEC(v) InterlockedExchangeAddNoFence64(v, -1)
#define AT_THPOOL_SHEDYIELD SwitchToThread
#else
#define AT_THPOOL_INC(v) __sync_fetch_and_add(v, 1)
#define AT_THPOOL_DEC(v) __sync_fetch_and_add(v, -1)
#define AT_THPOOL_SHEDYIELD sched_yield
#endif

typedef struct {
    void (*do_work)(void *);
    void *args;
} at_thtask_t;

struct at_thpool_s {
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
	pthread_t *threads;
#elif defined _WIN32 || _WIN64
	HANDLE *threads;
#endif
    lfqueue_t taskqueue;
    size_t nrunning;
    int is_running;
};


#if defined _WIN32 || defined _WIN64
unsigned __stdcall at_thpool_worker(void *);
#else
void* at_thpool_worker(void *);
#endif

at_thpool_t *
at_thpool_create(size_t nthreads) {
    size_t i;
    if (nthreads > MAX_THREADS) {
        fprintf(stderr, "The nthreads is > %d, over max thread will affect the system scalability, it might not scale well\n", MAX_THREADS);
    }

    at_thpool_t *tp;
    tp = (at_thpool_t*) AT_THPOOL_MALLOC(sizeof(at_thpool_t));
    if (tp == NULL) {
        AT_THPOOL_ERROR("malloc");
        return NULL;
    }

#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
	tp->threads = (pthread_t *)AT_THPOOL_MALLOC(sizeof(pthread_t) * nthreads);
#elif defined _WIN32 || _WIN64
	tp->threads = (HANDLE)AT_THPOOL_MALLOC(sizeof(HANDLE) * nthreads);
#endif

    tp->nrunning = 0;
    if (tp->threads == NULL) {
        AT_THPOOL_ERROR("malloc");
        AT_THPOOL_FREE(tp);
        return NULL;
    }

    if (lfqueue_init(&tp->taskqueue) < 0) {
        AT_THPOOL_ERROR("malloc");
        AT_THPOOL_FREE(tp->threads);
        AT_THPOOL_FREE(tp);
        return NULL;
    }

    tp->is_running = 1;

#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
    for (i = 0; i < nthreads; i++) {
        if (pthread_create(&(tp->threads[i]), NULL,  at_thpool_worker, (void*)tp)) {
            if (i != 0) {
                fprintf(stderr, "maximum thread has reached %zu \n", i );
                break;
            } else {
                AT_THPOOL_ERROR("Failed to establish thread pool");
                at_thpool_immediate_shutdown(tp);
            }
        }
        pthread_detach(tp->threads[i]);
    }
#elif defined _WIN32 || _WIN64
	for (i = 0; i < nthreads; i++) {
		unsigned udpthreadid;
		tp->threads[i] = (HANDLE)_beginthreadex(NULL, 0, at_thpool_worker, (void *)tp, 0, &udpthreadid);
		if (tp->threads[i] == 0) {
			if (i != 0) {
				fprintf(stderr, "maximum thread has reached %lld \n", i);
			}
			else {
				AT_THPOOL_ERROR("Failed to establish thread pool");
				at_thpool_immediate_shutdown(tp);
			}
		}
		CloseHandle(tp->threads[i]);
	}
#endif
    return tp;
}

#if defined _WIN32 || defined _WIN64
unsigned __stdcall at_thpool_worker(void *_tp) {
#else
void* at_thpool_worker(void *_tp) {
#endif
    at_thpool_t *tp = (at_thpool_t*)_tp;
    AT_THPOOL_INC(&tp->nrunning);

    at_thtask_t *task;
    void *_task;
    lfqueue_t *tq = &tp->taskqueue;
    int i;

TASK_PENDING:
    while (tp->is_running) {
        for (i = 0; i < DEF_SPIN_LOCK_CYCLE; i++) {
            if ( (_task = lfqueue_deq(tq)) ) {
                goto HANDLE_TASK;
            }
            // lfqueue_sleep(1); // lfqueue_deq will sleep for 1ms if not found
        }
        AT_THPOOL_SHEDYIELD();
    }

    AT_THPOOL_DEC(&tp->nrunning);
#if defined _WIN32 || defined _WIN64
	return 0;
#else
    return NULL;
#endif
HANDLE_TASK:
    task = (at_thtask_t*) _task;
    task->do_work(task->args);
    AT_THPOOL_FREE(task);
    goto TASK_PENDING;
}

int
at_thpool_newtask(at_thpool_t *tp, void (*task_pt)(void *), void *arg) {
    at_thtask_t *task = AT_THPOOL_MALLOC(sizeof(at_thtask_t));

    if (task == NULL) {
        AT_THPOOL_ERROR("malloc");
        return errno;
    }

    task->do_work = task_pt;
    task->args = arg;

    if (lfqueue_enq(&tp->taskqueue, task) == -1) {
        fprintf(stderr, "Task unable to assigned to pool, it might be full\n");
        AT_THPOOL_FREE(task);
        return -1;
    }
    return 0;
}

void
at_thpool_gracefully_shutdown(at_thpool_t *tp) {
    tp->is_running = 0;
    int i, ncycle = DEF_SPIN_LOCK_CYCLE;
    for (;;) {
        for (i = 0; i < ncycle; i++) {
            if (tp->nrunning == 0) {
                goto SHUTDOWN;
            }
        }
        AT_THPOOL_SHEDYIELD();
    }
SHUTDOWN:
    AT_THPOOL_SHEDYIELD();
    AT_THPOOL_FREE(tp->threads);
    lfqueue_destroy(&tp->taskqueue);
    AT_THPOOL_FREE(tp);

}

void
at_thpool_immediate_shutdown(at_thpool_t *tp) {
    tp->is_running = 0;
    AT_THPOOL_FREE(tp->threads);
    lfqueue_destroy(&tp->taskqueue);
    AT_THPOOL_FREE(tp);
}
