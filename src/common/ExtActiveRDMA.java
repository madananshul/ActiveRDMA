package common;

import java.io.File;
import java.io.FileInputStream;

public abstract class ExtActiveRDMA implements ActiveRDMA{
	
	/*
	 * Allocation function
	 */
	
	//this integer is the RESERVED position for counting allocated blocks
	//should not be used for anything else... or we will have bugs.
	//FIXME: memory is never reclaimed! dealloc()?
	static final int MEM_COUNTER = 0;
	public int alloc(int size){
		int c = r(MEM_COUNTER);
		//retry until we were able to increase the claimed memory by 'size'
		while( cas(MEM_COUNTER,c,c+size) == 0 ){
			//failed, get the new value
			c = r(MEM_COUNTER);
		} //TODO: should this be on the server side instead?
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
