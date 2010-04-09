package server;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;

import common.ActiveRDMA;
import common.ByteArray;
import common.messages.MessageFactory;
import common.messages.MessageVisitor;
import common.messages.MessageFactory.CAS;
import common.messages.MessageFactory.ErrorCode;
import common.messages.MessageFactory.Load;
import common.messages.MessageFactory.Operation;
import common.messages.MessageFactory.Read;
import common.messages.MessageFactory.Result;
import common.messages.MessageFactory.Run;
import common.messages.MessageFactory.Write;

public class Server extends ActiveRDMA implements MessageVisitor<DatagramPacket>
{
	
	final protected AtomicInteger[] memory;
	final protected BlockingQueue<Job> queue;
	final protected DatagramSocket socket;
	final protected MobileClassLoader loader;
	
	class Job{
		long timestamp;
		InetAddress address;
		int port;
		byte[] md5;
		int[] arg;
		
		public Job(long timestamp, InetAddress address, int port, byte[] md5, int[] arg){
			this.timestamp = timestamp;
			this.address = address;
			this.port = port;
			this.md5 = md5;
			this.arg = arg;
		}
	}
	
	public Server(int memory_size, int workers) throws IOException {
		super();
		loader = new MobileClassLoader();
		socket = new DatagramSocket(ActiveRDMA.SERVER_PORT);
		memory = new AtomicInteger[memory_size];
		for(int i=0; i<memory.length; ++i)
			memory[i] = new AtomicInteger(0);
		Alloc.init(this);
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
		
		//TODO: define a proper boundary for timeouts...
		if( (System.currentTimeMillis()-job.timestamp) > ActiveRDMA.REQUEST_TIMEOUT )
			return;
		
		Result result = _run(job.md5,job.arg);

		ByteArrayOutputStream oub = new ByteArrayOutputStream();
		DataOutputStream out = new DataOutputStream(oub);
		result.write(out);
		out.close();
		
		byte[] bb = oub.toByteArray();
		DatagramPacket res = new DatagramPacket(bb,bb.length);
		res.setAddress(job.address);
		res.setPort(job.port);
		
		socket.send(res);
	}
	
	public void listen() throws IOException{
		byte[] b = new byte[socket.getReceiveBufferSize()];
		DatagramPacket packet = new DatagramPacket(b, b.length);
		while(true){
			try {
				socket.receive(packet);
				
				DataInputStream in = new DataInputStream(new ByteArrayInputStream(packet.getData()));
				Operation op = MessageFactory.read(in);
				Result result = op.visit(this, packet);
				
				if( result == null )
					continue;

				ByteArrayOutputStream oub = new ByteArrayOutputStream();
				DataOutputStream out = new DataOutputStream(oub);
				result.write(out);
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
	
	public Result visit(Read read, DatagramPacket context) {
		return _r(read.address,read.size);
	}

	public Result visit(Write write, DatagramPacket context) {
		return _w(write.address,write.values);
	}

	public Result visit(CAS cas, DatagramPacket context) {
		return _cas(cas.address,cas.test,cas.value);
	}

	public Result visit(Run run, DatagramPacket context) {
		Job job = new Job(System.currentTimeMillis(),context.getAddress(),context.getPort(),run.md5,run.arg);
		try {
			queue.put(job);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return null;
	}

	public Result visit(Load load, DatagramPacket context) {
		return _load(load.code);
	}

	/*
	 * ActiveRDMA stuff
	 */
	
	public Result _cas(int address, int test, int value) {
		Result result = new Result();
		try{
			result.result = new int[]{memory[address].compareAndSet(test, value) ? 1 : 0};
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = new int[]{address};
		}
		return result;
	}

	public Result _load(byte[] code) {
		Result result = new Result();
		try{
			Class<?> c = loader.loadMobileCode( code );
			//indexes by the name of the class TODO: index by md5 instead?
			//this will actually never return false, if there is a previous
			//class with the same name LinkageError will occur.
			result.result = new int[]{tableClassAndMd5(c,code) ? 1 : 0};
			result.error = ErrorCode.OK;
		}catch(LinkageError e){
			//problems loading
			result.error = ErrorCode.DUPLCIATED_CODE;
		}
		return result;
	}

	public Result _r(int address, int size) {
		Result result = new Result();
		try{
			result.result = new int[size];
			for(int i=0;i<size;++i)
				result.result[i] =  memory[address+i].get();
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = null;
		}
		return result;
	}

	public Result _run(byte[] md5, int[] arg) {
		Result result = new Result();
		//note that this is calling locally, thus should NOT be queued
		Class<?> c = md5_to_class.get(new ByteArray(md5));
		try {
			Method m = c.getMethod(ActiveRDMA.METHOD, ActiveRDMA.SIGNATURE);
			result.result =  new int[]{(Integer) m.invoke(null, new Object[]{this,arg})};
			result.error = ErrorCode.OK;
		} catch (Exception e) {
			e.printStackTrace();
			result.result = new int[]{-1};
			result.error = ErrorCode.ERROR;
		}
		return result;
	}

	public Result _w(int address, int[] values) {
		Result result = new Result();
		try{
			result.result = new int[values.length];
			for(int i=0;i<values.length;++i){
				result.result[i] = memory[address+i].get();
				memory[address+i].set(values[i]);
			}
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = null;
		}
		return result;
	}

}


/* OLD TCP SERVER STUFF

	final protected AtomicInteger[] memory;
	final protected Map<String,Class<?>> map;
	final protected BlockingQueue<Job> queue;
	
	class Job{
		Socket socket;
		String name;
		int[] arg;
		
		public Job(Socket socket, String name, int[] arg){
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
			Method m = c.getMethod(ActiveRDMA.METHOD, ActiveRDMA.SIGNATURE);
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
		ServerSocket socket = new ServerSocket(ActiveRDMA.SERVER_PORT);
		
		while(true){
			try {
				// using TCP simplifies reliability of messages sent
				// but requires accept/close for each request to avoid starvation
				// UDP may require (explicit) acknowledges?
				Socket incoming = socket.accept();

				DataInputStream in = new DataInputStream(incoming.getInputStream());
				Operation op = MessageFactory.read(in);
				int result = op.visit(this, incoming);
				
				// ARGHH ugly code!
				// don't close socket if it's a Run operation
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
*/	
