package tests;

import junit.framework.TestCase;
import client.Client;

import common.ActiveRDMA;

public class ClientTest extends TestCase {

	static public class MobileCodeTest {
		public static int execute(ActiveRDMA a, int[] i) {
			a.w(i[0], 42);
			return a.r(i[0]);
		}
	}
	
	//MainServer should be running before launching tests
	//only one test at each time or it might fail...
	
	final String server = "localhost";
	
	public void tests_R_W_CAS() throws Exception {
		ActiveRDMA client = new Client(server);

		client.w(0,0);
		assertEquals( client.r(0) , 0 );
		
		assertEquals( client.w(0,1234567) , 0 );
		assertEquals( client.r(0) , 1234567 );
		
		assertTrue( client.cas(0,1234567,42) != 0 );
		assertEquals( client.r(0) , 42 );
	}
	
	public void tests_Load_Run() throws Exception{
		ActiveRDMA client = new Client(server);

		client.w(7,0);
		assertEquals( client.r(7) , 0 );
		
		assertTrue( client.load(MobileCodeTest.class) != 0 );
		
		// sets to [7]42
		assertEquals( client.run(MobileCodeTest.class,new int[]{7}) , 42 );
		assertEquals( client.r(7) , 42 );

	}
}
