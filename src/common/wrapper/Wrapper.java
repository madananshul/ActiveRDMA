package common.wrapper;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;

import common.ActiveRDMA;

@Deprecated
public class Wrapper<T> {

	protected int size;
	protected ActiveRDMA x;

	public Wrapper(ActiveRDMA x, int size){
		this.x = x;
		this.size = size;
	}

	public T get(){
		return null;
	}

	public void write(T new_value){

	}

	public boolean cas(T compare, T new_value){
		return false;
	}

	/**
	 * @return size of structure as a byte[]
	 */
	public int size(){
		return 0;
	}

	static public byte[] toByteArray(Object obj) throws IOException{
		ByteArrayOutputStream bos = new ByteArrayOutputStream(); 
		ObjectOutputStream oos = new ObjectOutputStream(bos); 
		oos.writeObject(obj);
		oos.flush(); 
		oos.close(); 
		bos.close();
		byte [] data = bos.toByteArray();
		return data;
	}
	
	static public Object toObject(byte[] array) throws Exception{
		ByteArrayInputStream bos = new ByteArrayInputStream(array); 
		ObjectInputStream oos = new ObjectInputStream(bos); 
		Object result;
		result = oos.readObject();
		oos.close(); 
		bos.close();
		return result;
	}
}
