package common;

import java.io.File;
import java.io.FileInputStream;
import java.util.concurrent.atomic.AtomicInteger;

public abstract class ExtActiveRDMA implements ActiveRDMA{
	
	/*
	 * Allocation function
	 */
	

	//this integer is the RESERVED position for counting allocated blocks
	//should not be used for anything else... or we will have bugs.
	//FIXME: memory is never reclaimed! dealloc()?
	
	//@MobileCode //TODO: use annotations to, at runtime (up)load lib code?
	public static class Alloc{
		static final int MEM_COUNTER = 0;
		public static int execute(AtomicInteger[] mem, int size) {
			int c = mem[MEM_COUNTER].get();
			//retry until we were able to increase the claimed memory by 'size'
			while( !mem[MEM_COUNTER].compareAndSet(c, c+size) ){
				//failed, get the new value
				c = mem[MEM_COUNTER].get();
			}
			return c;
		}
	}
	
	//TODO: should this be on the server side instead?
	public int alloc(int size){
		int c = r(Alloc.MEM_COUNTER);
		//retry until we were able to increase the claimed memory by 'size'
		while( cas(Alloc.MEM_COUNTER,c,c+size) == 0 ){
			//failed, get the new value
			c = r(Alloc.MEM_COUNTER);
		}
		return c;
	}
	
	/*
	 * Simplification of load and run commands
	 */
	
	/**
	 * @param name - must be the full name for the resource, not modifications
	 * are done before calling getResource(name).
	 */
	public int load(String name){
		try {
			File classfile = new File( ExtActiveRDMA.class.getClassLoader().getResource(name).toURI() );

			int len = (int)(classfile.length());
			byte[] bytes = new byte[len];
			FileInputStream i = new FileInputStream(classfile);
			i.read(bytes, 0, len);
			i.close();

			return load(bytes);
		} catch (Exception e) {
			e.printStackTrace();
			return 0;
		}
	}

	public int run(Class<?> c, int arg) {
		return run(c.getName(),arg);
	}

	public int load(Class<?> c) {
		return load(c.getName().replaceAll("\\.", "/")+".class");
	}
}
