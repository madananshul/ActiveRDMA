package common;


public interface ActiveRDMA {
	
	final int PORT = 15712;
	enum OpCode{ READ, WRITE, CAS, RUN, LOAD};

	//mobile code interface should be:
	//static public int execute(AtomicInteger[] mem, int arg);
	
	//reads value @address
	int r(int address);
	
	//writes value @address ; returns old
	int w(int address, int value);
	
	//compare-and-swap if(test) writes value
	//FIXME: returns boolean, not old value
	boolean cas(int address, int test, int value);
	
	//runs previous loaded code (with its md5 as UID)
	//FIXME: revert to md5?
	int run(String name, int arg);
	
	//loads byte[] as a class
	// true if not overriding anything
	//FIXME: what to return? anything at all? needs to block to ensure
	//FIXME: that code was loaded...
	boolean load(byte[] code);
	
}
