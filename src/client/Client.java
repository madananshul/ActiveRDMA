package client;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;

import common.ActiveRDMA;
import common.ExtActiveRDMA;
import common.messages.MessageFactory;

public class Client extends ExtActiveRDMA{

	protected InetAddress server;
	
	public Client(String server) throws IOException{
		this.server = InetAddress.getByName(server);
	}
	
	public int cas(int address, int test, int value) {
		return exchange( MessageFactory.makeCAS(address, test, value));
	}

	public int w(int address, int value) {
		return exchange( MessageFactory.makeWrite(address, value));
	}
	
	public int r(int address) {
		return exchange( MessageFactory.makeRead(address));
	}

	public int run(String name, int[] arg) {
		return exchange( MessageFactory.makeRun(name, arg));
	}
	
	public int load(byte[] code) {
		return exchange( MessageFactory.makeLoad(code));
	}

	/*
	 * Communication stuff
	 */
	
	protected int exchange(MessageFactory.Operation op){
		int result = 0;
		try {
			//FIXME: creates a new socket for each request, a bit wasteful
			DatagramSocket socket = new DatagramSocket();
			
			ByteArrayOutputStream b = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream( b );
			//TODO: append UID?
			op.write(out);
			out.close();
			
			byte[] ar = b.toByteArray();
			DatagramPacket p = new DatagramPacket(ar, ar.length);

			p.setAddress(server);
			p.setPort(ActiveRDMA.SERVER_PORT);

			socket.send(p);
			
			int n_retry = 3;
			while( n_retry-- > 0 ){
				try{
				socket.setSoTimeout(ActiveRDMA.REQUEST_TIMEOUT);
				socket.receive(p);
				break;
				}catch(SocketTimeoutException e){
					//re send
					socket.send(p);
				}
			}
			
			if( n_retry == -1 ){
				throw new RuntimeException("The server is dead, Jim.");
			}
			
			DataInputStream in = new DataInputStream(new ByteArrayInputStream(p.getData()));
			result = in.readInt();
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		return result;
	}

}

/* OLD TCP Client

protected int tcp_exchange(MessageFactory.Operation op){
	int result = 0;
	try {
		Socket socket = new Socket(server, ActiveRDMA.SERVER_PORT);
		
		DataOutputStream out = new DataOutputStream(socket.getOutputStream());
		op.write(out);
		out.flush();
		
		//always just one int returned
		DataInputStream in = new DataInputStream(socket.getInputStream());
		result = in.readInt();
		
		out.close();
		in.close();
		socket.close();
	} catch (UnknownHostException e) {
		e.printStackTrace();
	} catch (IOException e) {
		e.printStackTrace();
	}
	return result;
}

*/