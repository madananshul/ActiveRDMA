

public class MainServer {

	public static void main(String[] args) throws Exception {
		server.Server server = new server.Server(2000,2);
//		server.UdpServer server = new server.UdpServer(2000,2);
		server.listen();
	}

}
