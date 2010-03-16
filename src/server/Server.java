package server;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;

import common.ActiveRDMA;
import common.messages.MessageFactory;
import common.messages.MessageVisitor;
import common.messages.MessageFactory.CAS;
import common.messages.MessageFactory.Load;
import common.messages.MessageFactory.Operation;
import common.messages.MessageFactory.Read;
import common.messages.MessageFactory.Run;
import common.messages.MessageFactory.Write;

public class Server extends ClassLoader implements MessageVisitor<Socket>
{

	final protected AtomicInteger[] memory;
	final protected Map<String,Class<?>> map;
	final protected BlockingQueue<Job> queue;
	
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
	
	public Server(int memory_size, int workers) {
		super();
		map = new TreeMap<String, Class<?>>();
		memory = new AtomicInteger[memory_size];
		for(int i=0; i<memory.length; ++i)
			memory[i] = new AtomicInteger(0);
		queue = new LinkedBlockingQueue<Job>();
		
		//the worker pool
		for(int i=0; i<workers; ++i){
			new Thread(){
				//worker stuff
				public void run(){
					while(true){
						try{
							work();
						}catch(Exception e){
							e.printStackTrace();
						}
					}
				}
			}.start();
		}
	}
	
	protected void work() throws Exception {
		Job job = queue.take();
		int result = 0;
		Class<?> c = map.get(job.name);
		try {
			Method m = c.getMethod("execute", new Class[]{AtomicInteger[].class,int.class});
			result = (Integer) m.invoke(null, new Object[]{memory,job.arg});
		} catch (Exception e) {
			e.printStackTrace();
		}

		DataOutputStream out = new DataOutputStream(job.socket.getOutputStream());
		out.writeInt(result);
		out.flush();

		out.close();
		job.socket.close();
	}
	
	public void listen() throws IOException{
		ServerSocket socket = new ServerSocket(ActiveRDMA.PORT);
		
		while(true){
			try {
				//FIXME: using TCP simplifies reliability of messages sent
				// but requires accept/close for each request to avoid starvation
				// UDP may require (explicit) acknowledges?
				Socket incoming = socket.accept();

				DataInputStream in = new DataInputStream(incoming.getInputStream());
				Operation op = MessageFactory.read(in);
				int result = op.visit(this, incoming);
				
				//FIXME: ARGHH ugly code!
				//don't close socket if it's a Run operation
				if( op instanceof MessageFactory.Run )
					continue;

				DataOutputStream out = new DataOutputStream(incoming.getOutputStream());
				out.writeInt(result);
				out.flush();
				
				in.close();
				out.close();
				incoming.close();
				
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
	
	public void udp_listen() throws IOException{
		DatagramSocket socket = new DatagramSocket(ActiveRDMA.PORT);
		byte[] b = new byte[6500]; //FIXME: not good...
		DatagramPacket packet = new DatagramPacket(b, b.length);
		while(true){
			try {
				socket.receive(packet);
				
				DataInputStream in = new DataInputStream(new ByteArrayInputStream(packet.getData()));
				Operation op = MessageFactory.read(in);
				int result = op.visit(this, null);
				
				//FIXME: ARGHH ugly code!
				//don't close socket if it's a Run operation
				if( op instanceof MessageFactory.Run )
					continue;

				ByteArrayOutputStream oub = new ByteArrayOutputStream();
				DataOutputStream out = new DataOutputStream(oub);
				out.writeInt(result);
				out.close();
				
				byte[] bb = oub.toByteArray();
				DatagramPacket res = new DatagramPacket(bb,bb.length);
				res.setAddress(packet.getAddress());
				res.setPort(packet.getPort());
				socket.send(res);
				
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	/*
	 * Visit each operation
	 */
	
	public int visit(Read read, Socket context) {
		return memory[read.address].get();
	}

	public int visit(Write write, Socket context) {
		int old = memory[write.address].get();
		memory[write.address].set(write.value);
		return old;
	}

	public int visit(CAS cas, Socket context) {
		return memory[cas.address].compareAndSet(cas.test, cas.value) ? 1 : 0;
	}

	public int visit(Run run, Socket context) {
		Job job = new Job(context,run.name,run.arg);
		try {
			queue.put(job);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return 0;
	}

	public int visit(Load load, Socket context) {
		Class<?> c = defineClass(null, load.code, 0, load.code.length);
		//indexes by the name of the class TODO: index by md5 instead?
		return map.put(c.getName(),c) == null ? 1 : 0;
	}

}
