package common.messages;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class MessageFactory {
	
	public static enum ErrorCode{ OK, OUT_OF_BOUNDS, TIME_OUT, UNKNOWN_CODE, DUPLCIATED_CODE, ERROR };
	enum OpCode{ READ, WRITE, CAS, RUN, LOAD, READBYTES, WRITEBYTES };
	
	static public Operation makeRead(int address, int size){
		Read r = new Read();
		r.address = address;
		r.size = size;
		return r;
	}
	
	static public Operation makeWrite(int address, int[] values){
		Write r = new Write();
		r.address = address;
		r.values = values;
		return r;
	}
	
	static public Operation makeCAS(int address, int test, int value){
		CAS r = new CAS();
		r.address = address;
		r.test = test;
		r.value = value;
		return r;
	}

	static public Operation makeRun(byte[] md5, int[] arg){
		Run r = new Run();
		r.md5 = md5;
		r.arg = arg;
		return r;
	}
	
	static public Operation makeLoad(byte[] code){
		Load r = new Load();
		r.code = code;
		return r;
	}

    static public Operation makeReadBytes(int address, int count)
    {
        ReadBytes r = new ReadBytes();
        r.address = address;
        r.count = count;
        return r;
    }

    static public Operation makeWriteBytes(int address, byte[] data)
    {
        WriteBytes r = new WriteBytes();
        r.address = address;
        r.count = count;
        return r;
    }
	
	static private final OpCode[] code_table = OpCode.values();
	static public Operation read(DataInputStream in) throws IOException{
		int code = in.readInt();
		Operation result = null;

		switch(code_table[code]){
		case   CAS: result = new CAS();   break;
		case  READ: result = new Read();  break;
		case WRITE: result = new Write(); break;
		case  LOAD: result = new Load();  break;
		case   RUN: result = new Run();   break;
        case READBYTES: result = new ReadBytes(); break;
        case WRITEBYTES: result = new WriteBytes(); break;
		default:
			throw new RuntimeException("unexpected op code:" + code_table[code]);
		}
		result.read(in);
		return result;
	}
	
	/*
	 * 
	 */
	
	protected static void writeArray(DataOutputStream o, int[] array) throws IOException{
		o.writeInt( array == null ? -1 : array.length );
		if( array!= null )
			for(int i = 0 ; i < array.length ; ++i )
				o.writeInt(array[i]);
	}

    protected static void writeArray(DataOutputStream o, byte[] array) throws IOException {
        o.writeInt( array == null ? -1 : array.length );
        if (array != null)
            for (int i = 0; i < array.length; i++)
                o.writeByte(array[i]);
    }
	
	protected static int[] readArray(DataInputStream s) throws IOException {
		int n = s.readInt();
		if( n == -1 )
			return null;
		else{
			int[] result = new int[n];
			for(int i=0;i<result.length;++i)
				result[i] = s.readInt();
			return result;
		}
	}

    protected static byte[] readArray_b(DataInputStream s) throws IOException {
        int n = s.readInt();
        if (n == -1)
            return null;
        else
        {
            byte[] result = new byte[n];
            for (int i = 0; i <result.length; i++)
                result[i] = s.readByte();
            return result;
        }
    }
	
	public static class Result {
		public int[] result;
        public byte[] result_b;
		public ErrorCode error;
		
		public Result(){
			error = ErrorCode.ERROR;
			result = null;
            result_b = null;
		}
		
		public Result(int[] values){
			result = values;
			error = ErrorCode.OK;
		}

        public Result(byte[] values){
            result_b = values;
            error = ErrorCode.OK;
        }
		
		public Result(ErrorCode prob){
			if( prob == ErrorCode.OK )
				throw new IllegalArgumentException("Should not be OK.");
			error = prob;
			result = null;
		}

		public void write(DataOutputStream s) throws IOException {
			s.writeInt(error.ordinal());
			writeArray(s, result);
            writeArray(s, result_b);
		}
		
		public void read(DataInputStream s) throws IOException {
			error = ErrorCode.values()[s.readInt()];
			result = readArray(s);
            result_b = readArray_b(s);
		}
	}

	public static interface Operation {
		<T> Result visit(MessageVisitor<T> v, T context);

		void write(DataOutputStream s) throws IOException;
		void read(DataInputStream s) throws IOException;
	}
	
	public static class Read implements Operation {
		public int size;
		public int address;

		public <C> Result visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}

		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.READ.ordinal());
			s.writeInt(address);
			s.writeInt(size);
		}
		
		public void read(DataInputStream s) throws IOException {
			address = s.readInt();
			size = s.readInt();
		}
	}
	
	public static class Write implements Operation {
		public int address;
		public int[] values;
	
		public <C> Result visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}
		
		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.WRITE.ordinal());
			s.writeInt(address);
			writeArray(s, values);
		}
		
		public void read(DataInputStream s) throws IOException {
			address = s.readInt();
			values = readArray(s);
		}
	}
	
	public static class CAS implements Operation {
		public int address;
		public int test;
		public int value;
		
		public <C> Result visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}
		
		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.CAS.ordinal());
			s.writeInt(address);
			s.writeInt(test);
			s.writeInt(value);
		}
		
		public void read(DataInputStream s) throws IOException {
			address = s.readInt();
			test = s.readInt();
			value = s.readInt();
		}
	}
	
	public static class Run implements Operation {
		public byte[] md5;
		public int[] arg;
		
		public <C> Result visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}

		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.RUN.ordinal());
			s.writeInt(md5.length);
			for(int i=0;i<md5.length;++i)
				s.writeByte(md5[i]);
			writeArray(s, arg);
		}

		public void read(DataInputStream s) throws IOException {
			int n = s.readInt();
			md5 = new byte[n];
			for(int i=0;i<md5.length;++i)
				md5[i] = s.readByte();

			arg = readArray(s);
		}
	}
	
	public static class Load implements Operation {
		public byte[] code;
		
		public <C> Result visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}
		
		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.LOAD.ordinal());
			s.writeInt(code.length);
			s.write(code);
		}
		
		public void read(DataInputStream s) throws IOException {
			code = new byte[s.readInt()];
			s.read(code);
		}
	}

    public static class ReadBytes implements Operation {
        public int address, count;

        public <C> Result visit(MessageVisitor<C> v, C context) {
            return v.visit(this, context);
        }

        public void write(DataOutputStream s) throws IOException {
            s.writeInt(OpCode.READBYTES.ordinal());
            s.writeInt(address);
            s.writeInt(count);
        }

        public void read(DataInputStream s) throws IOException {
            address = s.readInt();
            count = s.readInt();
        }
    }

    public static class WriteBytes implements Operation {
        public int address;
        public byte[] data;

        public <C> Result visit(MessageVisitor<C> v, C context) {
            return v.visit(this, context);
        }

        public void write(DataOutputStream s) throws IOException {
            s.writeInt(OpCode.WRITEBYTES.ordinal());
            s.writeInt(address);
            writeArray(s, data);
        }

        public void read(DataInputStream s) throws IOException {
            address = s.readInt();
            data = readArray_b(s);
        }

    }
}
