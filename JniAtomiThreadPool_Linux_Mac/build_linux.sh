#!/bin/sh

gcc -shared -o libatpool_jni.so -fPIC -I$JAVA_HOME/include -I$JAVA_HOME/include/linux -I$JAVAHOME/include/darwin -I/usr/local/include -I/usr/include -I../ -I../lfqueue -I../jni-cross-platform ../lfqueue/lfqueue.c ../at_thpool.c ../jni-cross-platform/AtomicThreadPool.c -pthread
