package common;

import java.io.File;
import java.io.FileInputStream;

import common.messages.MessageFactory.Result;

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
	public abstract Result _r(int address);
	
	/** Writes the value into the server's memory.
	 * @param address - offset of the memory location
	 * @param value - the new value to be store
	 * @return old value
	 */
	public abstract Result _w(int address, int value);
	
	/** Compare-And-Swap, if test is true it will atomically write value into 
	 * the memory at location address.
	 * @param address - offset of the memory location
	 * @param test - condition that must be true for the assignment to occur
	 * @param value - the new value to be store
	 * @return boolean as int (0 - false, else - true) with the test result
	 */
	public abstract Result _cas(int address, int test, int value);

	/*
	 * Active operations
	 */
	
	// mobile code interface expected to be:
	// static public int execute(ActiveRDMA ardma, int arg); 
	final public static String METHOD = "execute";
	final public static Class<?>[] SIGNATURE = new Class[]{ActiveRDMA.class,int[].class};

	/** Executes the previously loaded code in the server.
	 * @param name - class to be executed, FIXME: this will change to something else, md5 maybe?
	 * @param arg - argument supplied to the executing code
	 * @result result of the executed method.
	 */
	public abstract Result _run(String name, int[] arg);
	
	/** Loads the code of a class
	 * @param code - the bytecode of a class, not its serialization!
	 * @return boolean as int (0 - false, else - true) with load success result
	 */ //TODO: the return value is not all that well defined, what is success?
	public abstract Result _load(byte[] code);
	
	/*
	 * exception based wrappers
	 */
	
	protected int unwrap(Result r){
		switch(r.error){
		case OUT_OF_BOUNDS:
		case TIME_OUT:
		case UNKNOWN_CODE:
		case DUPLCIATED_CODE:
		case ERROR:
			throw new RuntimeException("Error: "+r.error);
		case OK:
		default:
		}
		return r.result;
	}
	
	public int r(int address){
		return unwrap( _r(address) );
	}
	
	public int w(int address, int value){
		return unwrap( _w(address,value) );
	}
	
	public int cas(int address, int test, int value){
		return unwrap( _cas(address,test,value) );		
	}
	
	public int run(String name, int[] arg){
		return unwrap( _run(name,arg) );	
	}
	
	public int load(byte[] code){
		return unwrap( _load(code) );
	}
	
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
	
	public static class Alloc{
		static final int MEM_COUNTER = 0;
		
		public static void init(ActiveRDMA a){
			a.w(MEM_COUNTER, 1);
		}
		
		public static int execute(ActiveRDMA a, int[] args) {
			int size = args[0];
			int c = a.r(Alloc.MEM_COUNTER);
			//retry until we were able to increase the claimed memory by 'size'
			while( a.cas(Alloc.MEM_COUNTER,c,c+size) == 0 ){
				//failed, get the new value
				c = a.r(Alloc.MEM_COUNTER);
			}
			//System.out.println("Memcounter "+Alloc.MEM_COUNTER+" allocated "+c);
			return c;
		}
	}
	
	public int alloc(int size){
		return Alloc.execute(this, new int[]{size});
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
