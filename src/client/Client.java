package client;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

import common.ActiveRDMA;

public class Client implements ActiveRDMA{

	protected String server;
	
	public Client(String server){
		this.server = server;
	}

	//FIXME: code repetition is ugly, wish Java had lambdas...
	//FIXME: same problem with excessive socket open/closes
	
	public boolean cas(int address, int test, int value) {
		int result = 0;
		try {
			Socket socket = new Socket(server, ActiveRDMA.PORT);
			
			DataOutputStream out = new DataOutputStream(socket.getOutputStream());
			out.writeInt(OpCode.CAS.ordinal());
			out.writeInt(address);
			out.writeInt(test);
			out.writeInt(value);
			out.flush();
			
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
		return result != 0;
	}

	public int w(int address, int value) {
		int result = 0;
		try {
			Socket socket = new Socket(server, ActiveRDMA.PORT);
			
			DataOutputStream out = new DataOutputStream(socket.getOutputStream());
			out.writeInt(OpCode.WRITE.ordinal());
			out.writeInt(address);
			out.writeInt(value);
			out.flush();
			
			DataInputStream in = new DataInputStream(socket.getInputStream());
			result = in.read();
			
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
	
	public int r(int address) {
		int result = 0;
		try {
			Socket socket = new Socket(server, ActiveRDMA.PORT);
			
			DataOutputStream out = new DataOutputStream(socket.getOutputStream());
			out.writeInt(OpCode.READ.ordinal());
			out.writeInt(address);
			out.flush();
			
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

	public int run(String name, int arg) {
		int result = 0;
		try {
			Socket socket = new Socket(server, ActiveRDMA.PORT);
			
			DataOutputStream out = new DataOutputStream(socket.getOutputStream());
			out.writeInt(OpCode.RUN.ordinal());
			out.writeUTF(name);
			out.writeInt(arg);
			out.flush();
			
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
	
	//TODO: actually a higher level method, not from the ActiveRDMA interface
	public boolean load(String name){
		//FIXME: may depend on the bin/ etc configuration...
		//FIXME: this is horrible for nested classes we need to find how to 
		// extract the code from loaded classes
		File classfile = new File("bin/"+name+".class");
		int len = (int)(classfile.length());
		byte[] bytes = new byte[len];
		try {
			FileInputStream i = new FileInputStream(classfile);
			i.read(bytes, 0, len);
			i.close();
		} catch (IOException e) {
			e.printStackTrace();
			return false;
		}
		return load(bytes);
	}

	public boolean load(byte[] code) {
		int result = 0;
		try {
			Socket socket = new Socket(server, ActiveRDMA.PORT);
			
			DataOutputStream out = new DataOutputStream(socket.getOutputStream());
			out.writeInt(OpCode.LOAD.ordinal());
			out.writeInt(code.length);
			out.write(code);
			out.flush();
			
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
		return result != 0;
	}

}
