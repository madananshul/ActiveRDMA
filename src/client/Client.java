package client;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URISyntaxException;
import java.net.UnknownHostException;

import common.ActiveRDMA;
import common.messages.MessageFactory;

public class Client implements ActiveRDMA{

	protected String server;
	
	public Client(String server){
		this.server = server;
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

	public int run(String name, int arg) {
		return exchange( MessageFactory.makeRun(name, arg));
	}
	
	public int load(byte[] code) {
		return exchange( MessageFactory.makeLoad(code));
	}
	
	//TODO: actually a higher level method, not from the ActiveRDMA interface
	public int load(String name){
		//FIXME: this is horrible for nested classes we need to find how to 
		// extract the code from loaded classes
		File classfile;
		try {
			classfile = new File( Client.class.getClassLoader().getResource(name+".class").toURI() );
		} catch (URISyntaxException e1) {
			e1.printStackTrace();
			return 0;
		}
		int len = (int)(classfile.length());
		byte[] bytes = new byte[len];
		try {
			FileInputStream i = new FileInputStream(classfile);
			i.read(bytes, 0, len);
			i.close();
		} catch (IOException e) {
			e.printStackTrace();
			return 0;
		}
		return load(bytes);
	}

	//FIXME: same problem with excessive TCP socket open/closes
	protected int exchange(MessageFactory.Operation op){
		int result = 0;
		try {
			Socket socket = new Socket(server, ActiveRDMA.PORT);
			
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
	
	protected int udp_exchange(MessageFactory.Operation op){
		int result = 0;
		try {
			DatagramSocket socket = new DatagramSocket();
			
			ByteArrayOutputStream b = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream( b );
			//TODO: append UID
			op.write(out);
			out.close();
			
			byte[] ar = b.toByteArray();
			DatagramPacket p = new DatagramPacket(ar, ar.length);
			//FIXME: cache this...
			p.setAddress(InetAddress.getByName(server));
			p.setPort(ActiveRDMA.PORT);
			
			socket.send(p);
			//TODO: add timeouts.
			
			socket.receive(p);
			DataInputStream in = new DataInputStream(new ByteArrayInputStream(p.getData()));
			result = in.readInt();
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		return result;
	}

}
