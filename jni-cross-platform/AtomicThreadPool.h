#pragma once
#ifndef _ATOMIC_THREADPOOL_H
#define _ATOMIC_THREADPOOL_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_taymindis_lockfree_AtomicThreadPool_init(JNIEnv *env, jobject obj, jint nthreads);
JNIEXPORT jboolean JNICALL Java_com_taymindis_lockfree_AtomicThreadPool_newTask(JNIEnv *env, jobject obj, jobject threadObj);
JNIEXPORT void JNICALL Java_com_taymindis_lockfree_AtomicThreadPool_shutdown(JNIEnv *env, jobject obj);

#ifdef __cplusplus
}
#endif

#endif