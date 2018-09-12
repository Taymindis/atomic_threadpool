package com.github.taymindis.test;

import com.github.taymindis.AtomicThreadPoolTask;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * Created by woonsh on 10/9/2018.
 */
public class MyTask implements AtomicThreadPoolTask {

    int a = -1;
    static AtomicInteger totalRun = null;

    public MyTask(int a) {
        if(totalRun == null) {
            totalRun = new AtomicInteger(0);
        }
        this.a = a;
    }

    public int getA() {
        return a;
    }

    public void setA(int a) {
        this.a = a;
    }

    public void runTask() {
        totalRun.getAndIncrement();
        System.out.println("Task is running at " + a + " with thread id " + Thread.currentThread().getId());
    }

    public static AtomicInteger getTotalRun() {
        return totalRun;
    }
}
