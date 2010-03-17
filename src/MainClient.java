
import java.util.concurrent.atomic.AtomicInteger;

import client.Client;

public class MainClient {

	public static class MobileCode {
		public static int execute(AtomicInteger[] mem, int[] i) {
			mem[i[0]].set(42);
			return mem[i[0]].get();
		}
	}
	
	public static void main(String[] args) throws Exception {
		Client client = new Client("localhost");
		
		System.out.println( client.r(2) );
		System.out.println( client.w(2,12345) );
		System.out.println( client.r(2) );
		System.out.println( client.cas(2,12345,666) ) ;
		System.out.println( client.r(2) );
		
		System.out.println( client.load(MobileCode.class) );
		System.out.println( client.run(MobileCode.class, new int[]{2}) );
		System.out.println( client.r(2) );
	}

}
