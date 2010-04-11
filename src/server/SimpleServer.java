package server;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicInteger;

import common.ActiveRDMA;
import common.ByteArray;
import common.messages.MessageFactory;
import common.messages.MessageVisitor;
import common.messages.MessageFactory.CAS;
import common.messages.MessageFactory.ErrorCode;
import common.messages.MessageFactory.Load;
import common.messages.MessageFactory.Operation;
import common.messages.MessageFactory.Read;
import common.messages.MessageFactory.Result;
import common.messages.MessageFactory.Run;
import common.messages.MessageFactory.Write;

public class SimpleServer extends ActiveRDMA implements MessageVisitor<Object>
{
	
	final protected AtomicInteger[] memory;
	final protected MobileClassLoader loader;
	
    public SimpleServer(int memory_size) throws IOException {
		super();
		loader = new MobileClassLoader();
		memory = new AtomicInteger[memory_size];
		for(int i=0; i<memory.length; ++i)
			memory[i] = new AtomicInteger(0);
		Alloc.init(this);
    }
	
    public byte[] serve(byte[] inBytes)
    {
        try
        {
            DataInputStream in = new DataInputStream(new ByteArrayInputStream(inBytes));
            Operation op = MessageFactory.read(in);
            Result result = op.visit(this, null);

            ByteArrayOutputStream oub = new ByteArrayOutputStream();
            DataOutputStream out = new DataOutputStream(oub);
            result.write(out);
            out.close();
            return oub.toByteArray();
        }
        catch (IOException e)
        {
            return null;
        }
    }
	
	/*
	 * Visit each operation
	 */
	
	public Result visit(Read read, Object context) {
		return _r(read.address,read.size);
	}

	public Result visit(Write write, Object context) {
		return _w(write.address,write.values);
	}

	public Result visit(CAS cas, Object context) {
		return _cas(cas.address,cas.test,cas.value);
	}

	public Result visit(Run run, Object context) {
        return _run(run.md5, run.arg);
	}

	public Result visit(Load load, Object context) {
		return _load(load.code);
	}

	/*
	 * ActiveRDMA stuff
	 */
	
	public Result _cas(int address, int test, int value) {
		Result result = new Result();
		try{
			result.result = new int[]{memory[address].compareAndSet(test, value) ? 1 : 0};
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = new int[]{address};
		}
		return result;
	}

	public Result _load(byte[] code) {
		Result result = new Result();
		try{
			Class<?> c = loader.loadMobileCode( code );
			//indexes by the name of the class TODO: index by md5 instead?
			//this will actually never return false, if there is a previous
			//class with the same name LinkageError will occur.
			result.result = new int[]{tableClassAndMd5(c,code) ? 1 : 0};
			result.error = ErrorCode.OK;
		}catch(LinkageError e){
			//problems loading
			result.error = ErrorCode.DUPLCIATED_CODE;
		}
		return result;
	}

	public Result _r(int address, int size) {
		Result result = new Result();
		try{
			result.result = new int[size];
			for(int i=0;i<size;++i)
				result.result[i] =  memory[address+i].get();
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = null;
		}
		return result;
	}

	public Result _run(byte[] md5, int[] arg) {
		Result result = new Result();
		//note that this is calling locally, thus should NOT be queued
		Class<?> c = md5_to_class.get(new ByteArray(md5));
		try {
			Method m = c.getMethod(ActiveRDMA.METHOD, ActiveRDMA.SIGNATURE);
			result.result =  new int[]{(Integer) m.invoke(null, new Object[]{this,arg})};
			result.error = ErrorCode.OK;
		} catch (Exception e) {
			e.printStackTrace();
			result.result = new int[]{-1};
			result.error = ErrorCode.ERROR;
		}
		return result;
	}

	public Result _w(int address, int[] values) {
		Result result = new Result();
		try{
			result.result = new int[values.length];
			for(int i=0;i<values.length;++i){
				result.result[i] = memory[address+i].get();
				memory[address+i].set(values[i]);
			}
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = null;
		}
		return result;
	}

}
