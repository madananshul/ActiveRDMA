package server;

import java.net.Socket;

//worker Job information
class Job{
	Socket socket;
	String name;
	int arg;
	
	public Job(Socket socket, String name, int arg){
		this.socket = socket;
		this.name = name;
		this.arg = arg;
	}
}

