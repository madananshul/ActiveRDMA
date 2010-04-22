package tests;

import junit.framework.TestCase;
import client.Client;

import common.ActiveRDMA;

import examples.dht.DHT;
import examples.dht.DHT_Active;
import examples.dht.DHT_RDMA;

public class DHTTest extends TestCase {

    final String server = "localhost";

    final int TestCount = 1000;

    void runTiming(DHT d, String name)
    {
        long t1 = System.currentTimeMillis();

        for (int i = 0; i < TestCount; i++)
        {
            String s = String.format("key_%d", i);
            d.put(s, i);
        }

        long t2 = System.currentTimeMillis();

        for (int i = 0; i < TestCount; i++)
        {
            String s = String.format("key_%d", i);
            assertEquals(d.get(s), i);
        }

        long t3 = System.currentTimeMillis();

        double avgPut = (double)(t2 - t1) / TestCount;
        double avgGet = (double)(t3 - t2) / TestCount;

        System.out.println(name + " put: " + avgPut + " ms");
        System.out.println(name + " get: " + avgGet + " ms");

    }

    public void tests_DHT_RDMA() throws Exception {
        ActiveRDMA client = new Client(server);

        DHT d;

        d = new DHT_RDMA(client, 1024);
        runTiming(d, "RDMA  ");
//        d = new DHT_Active(client);
//        runTiming(d, "Active");
    }

}
