package tests;

import junit.framework.TestCase;
import client.Client;

import common.ExtActiveRDMA;

import examples.dht.DHT;
import examples.dht.DHT_Active;
import examples.dht.DHT_RDMA;

public class DHTTest extends TestCase {

    final String server = "localhost";

    public void tests_DHT_RDMA() throws Exception {
        System.out.println("starting client");
        ExtActiveRDMA client = new Client(server);
        System.out.println("connected");

        DHT d = new DHT_RDMA(client, 1024);

        d.put("asdf", 42);
        assertEquals(d.get("asdf"), 42);

        d = new DHT_Active(client);

        d.put("asdf", 42);
        assertEquals(d.get("asdf"), 42);
    }

}