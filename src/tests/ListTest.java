package tests;

import junit.framework.TestCase;
import client.Client;

import common.ExtActiveRDMA;

import examples.list.List;
import examples.list.List_Active;
import examples.list.List_RDMA;

public class ListTest extends TestCase {

    final String server = "localhost";

    final int TestCount = 200;

    void runTiming(List l, String name) {
        long t1 = System.currentTimeMillis();

        for (int i = 0; i < TestCount; i++)
            l.add(i);

        long t2 = System.currentTimeMillis();

        for (int i = 0; i < TestCount; i++)
            assertEquals(l.get(i), i);

        long t3 = System.currentTimeMillis();

        double avgPut = (double)(t2 - t1) / TestCount;
        double avgGet = (double)(t3 - t2) / TestCount;

        System.out.println(name + " put: " + avgPut + " ms");
        System.out.println(name + " get: " + avgGet + " ms");
    }

    public void tests_List() throws Exception {
        ExtActiveRDMA client = new Client(server);

        List d;
        d = new List_RDMA(client);
        runTiming(d, "RDMA  ");
        d = new List_Active(client);
        runTiming(d, "Active");
    }
    
    public void tests_List_RDMA() throws Exception {
        ExtActiveRDMA client = new Client(server);
        
        List list = new List_RDMA(client);
        for (int i = 0; i < 10; i++)
        	list.add(i);
        
        for (int i = 0; i < 10; i++)
        	assertEquals(list.get(i), i);
        	
        assertEquals( list.get(10) , -1);
    }
    
    public void tests_List_Active() throws Exception {
        ExtActiveRDMA client = new Client(server);
        
        List list = new List_Active(client);
        for (int i = 0; i < 10; i++)
        	list.add(i);
        
        for (int i = 0; i < 10; i++)
        	assertEquals(list.get(i), i);
        	
        assertEquals( list.get(10) , -1);
    }

}