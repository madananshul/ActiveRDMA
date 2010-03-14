package server;

import java.io.DataOutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;

public class Worker extends Thread{

	protected BlockingQueue<Job> queue;
	protected AtomicInteger[] memory;
	protected Map<String,Class<?>> map;
	
	public Worker(BlockingQueue<Job> queue, AtomicInteger[] memory, Map<String,Class<?>> map){
		this.queue = queue;
		this.memory = memory;
		this.map = map;
	}
	
	public void run(){
		while(true){
			try{
				Job job = queue.take();
				int result = run(job.name,job.arg);

				DataOutputStream out = new DataOutputStream(job.socket.getOutputStream());
				out.writeInt(result);
				out.flush();
				
				out.close();
				job.socket.close();
			}catch(Exception e){
				e.printStackTrace();
			}
		}
	}

	public int run(String name, int arg) {
		Class<?> c = map.get(name);
		int result = -1;
		try {
			Method m = c.getMethod("execute", new Class[]{AtomicInteger[].class,int.class});
			result = (Integer) m.invoke(null, new Object[]{memory,arg});
		} catch (SecurityException e) {
			e.printStackTrace();
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (IllegalAccessException e) {
			e.printStackTrace();
		} catch (InvocationTargetException e) {
			e.printStackTrace();
		}
		return result;
	}
}
