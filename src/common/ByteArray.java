package common;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

public final class ByteArray{
	
	static public MessageDigest getDigest(){
		try {
			return MessageDigest.getInstance("MD5");
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
			return null;
		}
	}
	
    protected final byte[] data;
    
    public ByteArray(byte[] code, MessageDigest m){
    	m.reset();
    	m.update(code);
    	data = m.digest();
    }

    public ByteArray(byte[] hash){
        this.data = hash;
    }

    public byte[] array(){
    	return data;
    }
    
    @Override
    public boolean equals(Object other){
        if ( !(other instanceof ByteArray) )
            return false;
        return Arrays.equals(data, ((ByteArray)other).data);
    }

    @Override
    public int hashCode(){
        return Arrays.hashCode(data);
    }
}