package common;

import java.io.File;
import java.io.FileInputStream;
import java.util.concurrent.atomic.AtomicInteger;

//FIXME: there is no specified result for when the address is out of bounds!
//FIXME: or when the execution fails... use error handlers?
public abstract class ActiveRDMA {
	
	final public static int REQUEST_TIMEOUT = 5000; //5000ms = 5s
	final public static int SERVER_PORT = 15712;
	
	/*
	 * RDMA operations
	 */
	
	/** Reads from the server's memory.
	 * @param address - offset of the memory location
	 * @return value of memory location *at the time of the read*
	 */
	public abstract int r(int address);
	
	/** Writes the value into the server's memory.
	 * @param address - offset of the memory location
	 * @param value - the new value to be store
	 * @return old value
	 */
	public abstract int w(int address, int value);
	
	/** Compare-And-Swap, if test is true it will atomically write value into 
	 * the memory at location address.
	 * @param address - offset of the memory location
	 * @param test - condition that must be true for the assignment to occur
	 * @param value - the new value to be store
	 * @return boolean as int (0 - false, else - true) with the test result
	 */ //TODO: returning int simplifies the interface?
	public abstract int cas(int address, int test, int value);

	/*
	 * Active operations
	 */
	
	// mobile code interface expected to be:
	// static public int execute(AtomicInteger[] mem, int arg); 
	final public static String METHOD = "execute";
	final public static Class<?>[] SIGNATURE = new Class[]{ActiveRDMA.class,int[].class};

	/** Executes the previously loaded code in the server.
	 * @param name - class to be executed, FIXME: this will change to something else, md5 maybe?
	 * @param arg - argument supplied to the executing code
	 * @result result of the executed method.
	 */
	public abstract int run(String name, int[] arg);
	
	/** Loads the code of a class
	 * @param code - the bytecode of a class, not its serialization!
	 * @return boolean as int (0 - false, else - true) with load success result
	 */ //TODO: the return value is not all that well defined, what is success?
	public abstract int load(byte[] code);
	
	
	/* ==============================================================
	 * all these methods should just use/extend the above "interface"
	 * ==============================================================
	 */

	
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
			File classfile = new File( ActiveRDMA.class.getClassLoader().getResource(name).toURI() );

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

	public int run(Class<?> c, int[] arg) {
		return run(c.getName(),arg);
	}

	public int load(Class<?> c) {
		return load(c.getName().replaceAll("\\.", "/")+".class");
	}
	
	
}
