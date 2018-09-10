package com.taymindis.lockfree;

import java.io.IOException;

/**
 * Created by woonsh on 10/9/2018.
 */
public class AtomicThreadPool {
    private long threadpoolPtr = -1;
    private static String OS = System.getProperty("os.name").toLowerCase();

    public long getThreadpoolPtr() {
        return threadpoolPtr;
    }

    public void setThreadpoolPtr(long threadpoolPtr) {
        this.threadpoolPtr = threadpoolPtr;
    }

    public native boolean init(int nthreads);
    public native boolean newTask(AtomicThreadPoolTask o);

    public static boolean isWindows32() {

        return ( OS.indexOf("win") >= 0 &&  "32".equals(System.getProperty("sun.arch.data.model")) );

    }

    public static boolean isWindows64() {

        return (OS.indexOf("win") >= 0 &&  "64".equals(System.getProperty("sun.arch.data.model")));

    }

    public static boolean isMac() {

        return (OS.indexOf("mac") >= 0);

    }

    public static boolean isUnix() {

        return (OS.indexOf("nix") >= 0 || OS.indexOf("nux") >= 0 || OS.indexOf("aix") > 0 );

    }

    public static boolean isSolaris() {

        return (OS.indexOf("sunos") >= 0);

    }


    static {
        try {
            if (isWindows32()) {
                NativeUtils.loadLibraryFromJar("/JniAtomiThreadPool_VS32.dll");
            } else if (isWindows64()) {
//            System.load("C:\\msys64\\home\\woonsh\\git-project\\AtomicThreadPool\\x64\\Release\\AtomicThreadPool.dll");
                NativeUtils.loadLibraryFromJar("/JniAtomiThreadPool_VS64.dll");
            } else if(isMac()){
                // Pending build from source
//                NativeUtils.loadLibraryFromJar("/libatpool_jni.dylib");
            } else {
                NativeUtils.loadLibraryFromJar("/libatpool_jni.so");
            }
        } catch (IOException e) {
            e.printStackTrace(); // This is probably not the best way to handle exception :-)
        }
    }
}
