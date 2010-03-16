package common.messages;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import common.ActiveRDMA.OpCode;

//TODO: wish Java had structs... wasteful allocations?
public class MessageFactory {
	
	static public Operation makeRead(int address){
		Read r = new Read();
		r.address = address;
		return r;
	}
	
	static public Operation makeWrite(int address, int value){
		Write r = new Write();
		r.address = address;
		r.value = value;
		return r;
	}
	
	static public Operation makeCAS(int address, int test, int value){
		CAS r = new CAS();
		r.address = address;
		r.test = test;
		r.value = value;
		return r;
	}

	static public Operation makeRun(String name, int arg){
		Run r = new Run();
		r.name = name;
		r.arg = arg;
		return r;
	}
	
	static public Operation makeLoad(byte[] code){
		Load r = new Load();
		r.code = code;
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
		default:
			throw new RuntimeException("unexpected op code:" + code_table[code]);
		}
		result.read(in);
		return result;
	}
	
	/*
	 * 
	 */

	public static interface Operation {
		<T> int visit(MessageVisitor<T> v, T context);

		void write(DataOutputStream s) throws IOException;
		void read(DataInputStream s) throws IOException;
	}
	
	public static class Read implements Operation {
		public int address;

		public <C> int visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}

		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.READ.ordinal());
			s.writeInt(address);
		}
		
		public void read(DataInputStream s) throws IOException {
			address = s.readInt();
		}
	}
	
	public static class Write implements Operation {
		public int address;
		public int value;
	
		public <C> int visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}
		
		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.WRITE.ordinal());
			s.writeInt(address);
			s.writeInt(value);
		}
		
		public void read(DataInputStream s) throws IOException {
			address = s.readInt();
			value = s.readInt();
		}
	}
	
	public static class CAS implements Operation {
		public int address;
		public int test;
		public int value;
		
		public <C> int visit(MessageVisitor<C> v, C context) {
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
		public String name;
		public int arg;
		
		public <C> int visit(MessageVisitor<C> v, C context) {
			return v.visit(this,context);
		}

		public void write(DataOutputStream s) throws IOException {
			s.writeInt(OpCode.RUN.ordinal());
			s.writeUTF(name);
			s.writeInt(arg);
		}

		public void read(DataInputStream s) throws IOException {
			name = s.readUTF();
			arg = s.readInt();
		}
	}
	
	public static class Load implements Operation {
		public byte[] code;
		
		public <C> int visit(MessageVisitor<C> v, C context) {
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
}
