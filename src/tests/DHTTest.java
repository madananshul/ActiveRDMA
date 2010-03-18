package tests;

import java.util.concurrent.atomic.AtomicInteger;
import junit.framework.TestCase;

import common.ExtActiveRDMA;
import client.Client;
import examples.dht;



public class DHTTest extends TestCase {

    final String server = "localhost";

    public void tests_DHT_RDMA() throws Exception {
        ExtActiveRDMA client = new Client(server);

        DHT d = new DHT_RDMA(client, 1024);

        d.put("asdf", 42);
        assertEquals(d.get("asdf", 42));
    }

}