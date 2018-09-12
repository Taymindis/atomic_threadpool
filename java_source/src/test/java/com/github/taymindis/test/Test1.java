package com.github.taymindis.test;

import java.io.IOException;

import com.github.taymindis.AtomicThreadPool;
import org.junit.Test;
import static org.junit.Assert.*;
/**
 * Created by woonsh on 10/9/2018.
 */
public class Test1 {

    @Test
    public void testLoadLibraryIllegalPath() throws IOException, InterruptedException {
        System.out.println("Thread pool is running in 2 sec");
        Thread.sleep(2000);
        AtomicThreadPool p = new AtomicThreadPool();
        p.init(8);

        for(int i=0; i< 1000; i++) {
            assertTrue(p.newTask(new MyTask(i)));
        }

        Thread.sleep(3000);
        assertTrue(MyTask.getTotalRun().intValue() == 1000);
    }

}
