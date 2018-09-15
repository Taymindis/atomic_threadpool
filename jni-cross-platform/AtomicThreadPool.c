#include <jni.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include <excpt.h>
#include <at_thpool.h>

#define callStaticByMethod$(returnType, env, clazz, methodName, sig) \
(*env)->CallStatic##returnType##Method(env,clazz, (*env)->GetStaticMethodID(env, clazz, methodName, sig))
#define callStaticByMethod$2(returnType, env, clazz, methodName, sig, ... ) \
(*env)->CallStatic##returnType##Method(env,clazz, (*env)->GetStaticMethodID(env, clazz, methodName, sig), __VA_ARGS__)

#define callStaticByField$(returnType, env, clazz, fieldName, sig ) \
(*env)->GetStatic##returnType##Field(env, clazz, (*env)->GetStaticFieldID(env, clazz, fieldName, sig))

#define callByMethod$(returnType, env, clazz, obj$, methodName, sig ) \
(*env)->Call##returnType##Method(env, obj$, (*env)->GetMethodID(env, clazz, methodName, sig))
#define callByMethod$2(returnType, env, clazz, obj$, methodName, sig, ... ) \
(*env)->Call##returnType##Method(env, obj$, (*env)->GetMethodID(env, clazz, methodName, sig), __VA_ARGS__)

#define callByField$(returnType, env, clazz, obj$, fieldName, sig ) \
(*env)->Get##returnType##Field(env, obj$, (*env)->GetFieldID(env,clazz, fieldName, sig))

static JavaVM *jvm = NULL;				// Pointer to the JVM (Java Virtual Machine)
										//static JNIEnv *_env = NULL;
void update_curr_jvm(JNIEnv *env);

void runTask_(void* arg) {
	JNIEnv * _env;
	jint rs, getEnvStat = (*jvm)->GetEnv(jvm, (void **)&_env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		// No attached, attached now
		rs = (*jvm)->AttachCurrentThread(jvm, (void**)&_env, NULL);
		assert(rs == JNI_OK);
	}

	jobject refThreadObj = (jobject)arg;
	jclass clz = (*_env)->GetObjectClass(_env, refThreadObj);
	callByMethod$(Void, _env, clz, refThreadObj, "runTask", "()V");

	(*_env)->DeleteGlobalRef(_env, refThreadObj);
	(*_env)->DeleteLocalRef(_env, clz);

	rs = (*jvm)->DetachCurrentThread(jvm);
	assert(rs == JNI_OK);
}

JNIEXPORT jboolean JNICALL Java_com_github_taymindis_AtomicThreadPool_init(JNIEnv *env, jobject obj, jint nthreads) {
	if (jvm == NULL) {
		//_env = env;
		update_curr_jvm(env);
	}
	jclass clz = (*env)->GetObjectClass(env, obj);
	printf("%s, %d\n", "Intializing threadpool", nthreads);
	at_thpool_t *thpool = at_thpool_create(nthreads);

	callByMethod$2(Void, env, clz, obj, "setThreadpoolPtr", "(J)V", (jlong)thpool);

	/****
	* Local references are garbage collected when the native function returns to Java (when Java calls native) 
	* or when the calling thread is detached from the JVM (in native calls Java). 
	* You need explicit DeleteLocalRef only when you have a long lived native function 
	* (e.g., a main loop) or create a large number of transient objects in a loop
	* Since you are return JNI true, DeleteLocalRef is not necessary at this point.
	****/

	(*env)->DeleteLocalRef(env, clz);


	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_github_taymindis_AtomicThreadPool_newTask(JNIEnv *env, jobject obj, jobject threadObj) {
	jclass clz = (*env)->GetObjectClass(env, obj);
	at_thpool_t *thpool = (at_thpool_t *)callByField$(Long, env, clz, obj, "threadpoolPtr", "J");
	jobject refObj = (*env)->NewGlobalRef(env, threadObj);
	if (thpool != NULL && thpool > 0) {
		if (at_thpool_newtask(thpool, runTask_, refObj) != -1) {
			return JNI_TRUE;
		}
	}
	printf("The pool has not initialized yet");
	(*env)->DeleteLocalRef(env, clz);
	return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_github_taymindis_AtomicThreadPool_shutdown(JNIEnv *env, jobject obj) {
	jclass clz = (*env)->GetObjectClass(env, obj);
	at_thpool_t *thpool =(at_thpool_t *) callByMethod$(Long, env, clz, obj, "getThreadpoolPtr", "()J");
	if (thpool != NULL && thpool > 0) {
		at_thpool_gracefully_shutdown(thpool);
	}
	(*env)->DeleteLocalRef(env, clz);
}

void update_curr_jvm(JNIEnv *env) {
	if (!(*env)->GetJavaVM(env, &jvm) < 0) {
		return;
	}
	jint ver = (*env)->GetVersion(env); // (*env)->GetVersion();
										//cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) ;
										//jobject a = callStaticByMethod$2(Object, env, cls2, "getTag", "(I)LTestIvd;", 2); //(*env)->CallStaticObjectMethod(cls2, getTagStaicMethodId, 1);

										//cout << "Field tag id is " << callByMethod$(Int, env, cls2, a, "getId", "()I") << "\n";
	printf("Version is %d\n", ver);
}


