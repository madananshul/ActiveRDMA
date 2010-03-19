package server;

public class MobileClassLoader extends ClassLoader{

	public Class<?> loadMobileCode(byte[] code) throws LinkageError {
		return defineClass(null, code, 0, code.length);
	}
	
}
