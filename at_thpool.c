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

#if defined _WIN32 || _WIN64
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h> // for usleep
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

#if defined _WIN32 || _WIN64
#define AT_THPOOL_INC(v) InterlockedIncrement(v)
#define AT_THPOOL_DEC(v) InterlockedDecrement(v)
#define AT_THPOOL_SHEDYIELD SwitchToThread
#else
#define AT_THPOOL_INC(v) __atomic_fetch_add(v, 1, __ATOMIC_RELAXED)
#define AT_THPOOL_DEC(v) __atomic_fetch_add(v, -1, __ATOMIC_RELAXED)
#define AT_THPOOL_SHEDYIELD sched_yield
#endif

typedef struct {
    void (*do_work)(void *);
    void *args;
} at_thtask_t;

struct at_thpool_s {
    pthread_t *threads;
    lfqueue_t taskqueue;
    int nrunning;
    int is_running;
};


void* at_thpool_worker(void *);

at_thpool_t *at_thpool_create(int nthreads, int task_backlog) {
    int i;
    if (nthreads > MAX_THREADS) {
        fprintf(stderr, "The nthreads is > %d, over max thread will affect the system scalability, it might not scale well\n", MAX_THREADS);
    }

    if ( task_backlog < (nthreads * 6) ) {
        fprintf(stderr, "task_backlog should at least more than (nthreads * 6) \n");
        return NULL;
    }

    at_thpool_t *tp;
    tp = (at_thpool_t*) AT_THPOOL_MALLOC(sizeof(at_thpool_t));
    if (tp == NULL) {
        AT_THPOOL_ERROR("malloc");
        return NULL;
    }

    tp->threads = (pthread_t *)AT_THPOOL_MALLOC(sizeof(pthread_t) * nthreads);
    tp->nrunning = 0;
    if (tp->threads == NULL) {
        AT_THPOOL_ERROR("malloc");
        AT_THPOOL_FREE(tp);
        return NULL;
    }

    if (lfqueue_init(&tp->taskqueue, task_backlog) < 0) {
        AT_THPOOL_ERROR("malloc");
        AT_THPOOL_FREE(tp->threads);
        AT_THPOOL_FREE(tp);
        return NULL;
    }

    tp->is_running = 1;

    for (i = 0; i < nthreads; i++) {
        if (pthread_create(&(tp->threads[i]), NULL,  at_thpool_worker, (void*)tp)) {
            if (i != 0) {
                fprintf(stderr, "maximum thread has reached %d \n", i );
                break;
            } else {
                AT_THPOOL_ERROR("Failed to establish thread pool");
                at_thpool_immediate_shutdown(tp);
            }
        }
        pthread_detach(tp->threads[i]);
    }
    return tp;
}

void*
at_thpool_worker(void *_tp) {
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
        }
        AT_THPOOL_SHEDYIELD();
    }

    AT_THPOOL_DEC(&tp->nrunning);
    pthread_exit(NULL);
    return NULL;
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

    if (lfqueue_enq(&tp->taskqueue, task) != 1) {
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
