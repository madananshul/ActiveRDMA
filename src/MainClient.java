
import client.Client;

public class MainClient {

	public static void main(String[] args) throws Exception {
		Client client = new Client("localhost");
		
		System.out.println( client.r(2) );
		System.out.println( client.w(2,12345) );
		System.out.println( client.r(2) );
		System.out.println( client.cas(2,12345,666) ) ;
		System.out.println( client.r(2) );
		
		System.out.println( client.load("MobileCode") );
		System.out.println( client.run("MobileCode", 2) );
		System.out.println( client.r(2) );
	}

}
