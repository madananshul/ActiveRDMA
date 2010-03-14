
import server.Server;


public class MainServer {

	public static void main(String[] args) throws Exception {
		Server server = new Server(2000,2);
		server.listen();
	}

}
