package tests;

import client.Client;
import junit.framework.TestCase;

public class ClientTest extends TestCase {

	//MainServer should be running before launching tests
	//only one test at each time or it might fail...
	
	final String server = "localhost";
	
	public void tests_R_W_CAS() throws Exception {
		Client client = new Client(server);

		client.w(0,0);
		assertEquals( client.r(0) , 0 );
		
		assertEquals( client.w(0,1234567) , 0 );
		assertEquals( client.r(0) , 1234567 );
		
		assertEquals( client.cas(0,1234567,42) , true );
		assertEquals( client.r(0) , 42 );
	}
	
	public void tests_Load_Run() throws Exception{
		Client client = new Client(server);

		client.w(7,0);
		assertEquals( client.r(7) , 0 );
		
		// assuming bin/MobileCode.class exists ...
		assertEquals( client.load("MobileCode") , true );
		
		// sets to [7]42
		assertEquals( client.run("MobileCode",7) , 42 );
		assertEquals( client.r(7) , 42 );

	}
}
