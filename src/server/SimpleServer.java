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
import common.messages.MessageFactory.*;

public class SimpleServer extends ActiveRDMA implements MessageVisitor<Object>
{
	
	final protected byte[] memory;
	final protected MobileClassLoader loader;

    private long stat_rd, stat_wr, stat_cas;
	
    public SimpleServer(int memory_size) throws IOException {
		super();
		loader = new MobileClassLoader();
        memory = new byte[memory_size];
		Alloc.init(this);

        stat_rd = 0;
        stat_wr = 0;
        stat_cas = 0;
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

    public long getStat(int stat)
    {
        if (stat == 0) return stat_rd;
        if (stat == 1) return stat_wr;
        if (stat == 2) return stat_cas;
        return 0;
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

    public Result visit(ReadBytes readbytes, Object context) {
        return _readbytes(readbytes.address, readbytes.count);
    }

    public Result visit(WriteBytes writebytes, Object context) {
        return _writebytes(writebytes.address, writebytes.data);
    }

	/*
	 * ActiveRDMA stuff
	 */

    private int readW(int address) throws ArrayIndexOutOfBoundsException
    {
        return (int)memory[address] + (int)memory[address+1] << 8 +
            (int)memory[address+2] << 16 + (int)memory[address+3] << 24;
    }

    private void writeW(int address, int word) throws ArrayIndexOutOfBoundsException
    {
        memory[address] = (byte)(word & 0xff);
        memory[address+1] = (byte)((word >> 8) & 0xff);
        memory[address+2] = (byte)((word >> 16) & 0xff);
        memory[address+3] = (byte)((word >> 24) & 0xff);
    }
	
	public Result _cas(int address, int test, int value) {
        stat_cas++;
		Result result = new Result();
		try{
            int old = readW(address);
            if (old == test) writeW(address, value);
			result.result = new int[]{ (old == test) ? 1 : 0 };
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
        stat_rd += 4 * size;
		Result result = new Result();
		try{
			result.result = new int[size];
			for(int i=0;i<size;++i)
				result.result[i] = readW(address + 4*i);

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
        stat_wr += 4 * values.length;
		Result result = new Result();
		try{
			result.result = new int[values.length];
			for(int i=0;i<values.length;++i){
				result.result[i] = readW(address + 4*i);
                writeW(address + 4*i, values[i]);
			}
			result.error = ErrorCode.OK;
		}catch(ArrayIndexOutOfBoundsException exc){
			result.error = ErrorCode.OUT_OF_BOUNDS;
			result.result = null;
		}
		return result;
	}

    public Result _readbytes(int address, int count) {
        stat_rd += count;
        Result result = new Result();
        try {
            result.result_b = new byte[count];
            for (int i = 0; i < count; i++)
                result.result_b[i] = memory[address + i];
            result.error = ErrorCode.OK;
        } catch (ArrayIndexOutOfBoundsException exc) {
            result.error = ErrorCode.OUT_OF_BOUNDS;
            result.result_b = null;
        }
        return result;
    }

    public Result _writebytes(int address, byte[] values) {
        stat_wr += values.length;
        Result result = new Result();
        try {
            for (int i = 0; i < values.length; i++)
                memory[address + i] = values[i];
            result.error = ErrorCode.OK;
        } catch (ArrayIndexOutOfBoundsException exc) {
            result.error = ErrorCode.OUT_OF_BOUNDS;
            result.result_b = null;
        }
        return result;
    }

}
