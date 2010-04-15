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
        
				
		int table = lock_sys.create(10);
		
		int lock_offset = 1;
		int lock_id = lock_sys.lock(lock_offset, table+2*lock_offset);
		//sleep
		lock_sys.release(lock_id, table+2*lock_offset);
		
    }
    

}