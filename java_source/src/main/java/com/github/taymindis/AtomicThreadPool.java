package com.github.taymindis;

import java.io.IOException;
import java.util.logging.Logger;

/**
 * Created by woonsh on 10/9/2018.
 */
public class AtomicThreadPool {
    private long threadpoolPtr = -1;
    private static String OS = System.getProperty("os.name").toLowerCase();
    private static final Logger logger = Logger.getLogger(AtomicThreadPool.class.getName());

    public AtomicThreadPool(int nthreads) {
        try {
            if (!init(nthreads)) {
                throw new Exception("Unable to initialize thread pool correctly");
            }
        }catch (Exception e) {
            e.printStackTrace();
        }
    }

    public long getThreadpoolPtr() {
        return threadpoolPtr;
    }

    public void setThreadpoolPtr(long threadpoolPtr) {
        this.threadpoolPtr = threadpoolPtr;
    }

    private native boolean init(int nthreads);
    public native boolean newTask(AtomicThreadPoolTask o);
    public native boolean shutdown();

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
                NativeUtils.loadLibraryFromJar("/VS32.dll");
            } else if (isWindows64()) {
//            System.load("C:\\msys64\\home\\woonsh\\git-project\\AtomicThreadPool\\x64\\Release\\AtomicThreadPool.dll");
                NativeUtils.loadLibraryFromJar("/VS64.dll");
            } else if(isMac()){
                // Pending build from source
                logger.warning(" Mac is not support yet....");
//                NativeUtils.loadLibraryFromJar("/libatpool_jni.dylib");
            } else {
                NativeUtils.loadLibraryFromJar("/libatpool_jni.so");
            }
        } catch (IOException e) {
            e.printStackTrace(); // This is probably not the best way to handle exception :-)
        } catch (UnsatisfiedLinkError e) {
            /** It could be no dependencies found, load full pack **/
            logger.warning(" no dependencies found, load full pack library....");
            try {
                if (isWindows32()) {
                    NativeUtils.loadLibraryFromJar("/VS32_full.dll");
                } else if (isWindows64()) {
                    NativeUtils.loadLibraryFromJar("/VS64_full.dll");
                } else if(isMac()){
                    logger.warning(" Mac is not support yet....");
                    //                NativeUtils.loadLibraryFromJar("/libatpool_jni.dylib");
                } else {
                    NativeUtils.loadLibraryFromJar("/libatpool_jni.so");
                }
            } catch (IOException xe) {
            }
        }
    }
}
