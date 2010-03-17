package common;

import java.util.concurrent.atomic.AtomicInteger;

//FIXME: there is no specified result for when the address is out of bounds!
//FIXME: or when the execution fails... use error handlers?
public interface ActiveRDMA {
	
	final public int REQUEST_TIMEOUT = 5000; //5000ms = 5s
	final public int SERVER_PORT = 15712;

	/*
	 * RDMA operations
	 */
	
	/** Reads from the server's memory.
	 * @param address - offset of the memory location
	 * @return value of memory location *at the time of the read*
	 */
	int r(int address);
	
	/** Writes the value into the server's memory.
	 * @param address - offset of the memory location
	 * @param value - the new value to be store
	 * @return old value
	 */
	int w(int address, int value);
	
	/** Compare-And-Swap, if test is true it will atomically write value into 
	 * the memory at location address.
	 * @param address - offset of the memory location
	 * @param test - condition that must be true for the assignment to occur
	 * @param value - the new value to be store
	 * @return boolean as int (0 - false, else - true) with the test result
	 */ //TODO: returning int simplifies the interface?
	int cas(int address, int test, int value);

	/*
	 * Active operations
	 */
	
	// mobile code interface expected to be:
	// static public int execute(AtomicInteger[] mem, int arg); 
	final public String METHOD = "execute";
	final public Class<?>[] SIGNATURE = new Class[]{AtomicInteger[].class,int[].class};

	/** Executes the previously loaded code in the server.
	 * @param name - class to be executed, FIXME: this will change to something else, md5 maybe?
	 * @param arg - argument supplied to the executing code
	 * @result result of the executed method.
	 */
	int run(String name, int[] arg);
	
	/** Loads the code of a class
	 * @param code - the bytecode of a class, not its serialization!
	 * @return boolean as int (0 - false, else - true) with load success result
	 */ //TODO: the return value is not all that well defined, what is success?
	int load(byte[] code);
	
}
