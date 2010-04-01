package tests;

import junit.framework.TestCase;
import client.Client;

import common.ActiveRDMA;
import examples.lock.*;

public class LockTest extends TestCase {

    final String server = "localhost";

    final int TestCount = 200;

        
    public void tests_Lock_Active() throws Exception {
        ActiveRDMA client = new Client(server);
        
        Lock lock_sys = new Lock_Active(client);
        
        //Lock_Example = new Lock_Example();
				
		int table = lock_sys.create(10);
		//System.out.println("Lock table head is at address :"+table);
		
		int lock_offset = 1;
		int lock_id = lock_sys.lock(lock_offset, table+2*lock_offset);
		//sleep
		lock_sys.release(lock_id, table+2*lock_offset);
		
    }
    
    /*public void tests_Lock_Active_Threading() throws Exception {
        ActiveRDMA client = new Client(server);
        
        Lock lock_sys = new Lock_Active(client);
        
        //Lock_Example = new Lock_Example();
				
		int table = lock_sys.create(10);
		
		Thread t1 = new Thread(new Locker(1, sleep1));
        t1.start();	
        
        Thread t2 = new Thread((Runnable) new Locker(1, sleep2));
        t2.start();
		
    }*/
    
    /*private static class Locker implements Runnable {
        public void run() {
           
            try {
                for (int i = 0; i < importantInfo.length; i++) {
                    //Pause for 4 seconds
                    Thread.sleep(4000);
                    //Print a message
                    threadMessage(importantInfo[i]);
                }
            } catch (InterruptedException e) {
                threadMessage("I wasn't done!");
            }
        }
    }*/

}