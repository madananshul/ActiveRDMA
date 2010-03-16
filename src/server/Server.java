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

public class Server extends ClassLoader implements ActiveRDMA 
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
		final OpCode[] code_table = OpCode.values();
		
		while(true){
			try {
				//FIXME: using TCP simplifies reliability of messages sent
				// but requires accept/close for each request to avoid starvation
				// UDP may require (explicit) acknowledges?
				Socket incoming = socket.accept();

				DataInputStream in = new DataInputStream(incoming.getInputStream());
				int code = in.readInt();
				int result = 0;
				
				switch(code_table[code]){
				case CAS:
					result = cas(in.readInt(),in.readInt(),in.readInt());
					break;
				case READ:
					result = r(in.readInt());
					break;
				case WRITE:
					result = w(in.readInt(),in.readInt());
					break;

				case LOAD:
					byte[] array = new byte[in.readInt()];
					in.read(array);
					result = load(array);
					break;

				case RUN:
					Job job = new Job(incoming,in.readUTF(),in.readInt());
					
					//this closes the *whole* socket, so let it leak (will be
					//close together with the socket.close()
					// in.close();
					try {
						queue.put(job);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
					continue; 
					//note this jump to next *iteration* instead of doing
					//clean up stuff bellow
					
				default:
					throw new RuntimeException("OpCode "+code+" unexpected.");
				}

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
	
	//FIXME: error codes for array out of bound errors
	
	public int r(int address) {
		return memory[address].get();
	}

	public int w(int address, int value) {
		int old = memory[address].get();
		memory[address].set(value);
		return old;
	}
	
	public int cas(int address, int test, int value) {
		return memory[address].compareAndSet(test, value) ? 1 : 0;
	}

	public int load(byte[] code) {
		Class<?> c = defineClass(null, code, 0, code.length);
		//indexes by the name of the class TODO: index by md5 instead?
		return map.put(c.getName(),c) == null ? 1 : 0;
	}

	public int run(String name, int arg) {
		//FIXME: design bug, worker pool does not use this interface
		throw new RuntimeException("ok this is really a design bug");
	}

}
