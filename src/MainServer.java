

public class MainServer {

    // 128 MB (32M words) of server memory
    //static int Words = 32*1024*1024;
	static int Words = 32*1024;

	public static void main(String[] args) throws Exception {
		server.Server server = new server.Server(Words,2);
		server.listen();
	}

}
