package server;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
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

	protected ServerSocket socket;
	protected AtomicInteger[] memory;
	protected Map<String,Class<?>> map;
	protected BlockingQueue<Job> queue;
	
	public Server(int memory_size, int workers) throws IOException{
		super();
		map = new TreeMap<String, Class<?>>();
		memory = new AtomicInteger[memory_size];
		for(int i=0; i<memory.length; ++i)
			memory[i] = new AtomicInteger(0);
		
		socket = new ServerSocket(ActiveRDMA.PORT);
		
		queue = new LinkedBlockingQueue<Job>();
		
		//the worker pool
		Worker[] w = new Worker[workers];
		for(int i=0; i<workers; ++i)
			w[i] = new Worker(queue,memory,map);
		
		for(int i=0; i<workers; ++i)
			w[i].start();
	}
	
	public void listen(){

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
		
		//this closes the *whole* socket, so let it leak (will be
		//close together with the socket.close()
		// in.close();
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
