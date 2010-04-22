package client;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.util.HashMap;
import java.util.Map;

import common.ActiveRDMA;
import common.ByteArray;
import common.messages.MessageFactory;
import common.messages.MessageFactory.ErrorCode;
import common.messages.MessageFactory.Result;

public class Client extends ActiveRDMA{

	protected InetAddress server;
	protected Map<String,ByteArray> map;
	
	public Client(String server) throws IOException{
		this.server = InetAddress.getByName(server);
		this.map = new HashMap<String, ByteArray>();
	}
	
	public Result _cas(int address, int test, int value) {
		return exchange( MessageFactory.makeCAS(address, test, value));
	}

	public Result _w(int address, int[] values) {
		return exchange( MessageFactory.makeWrite(address, values));
	}
	
	public Result _r(int address, int size) {
		return exchange( MessageFactory.makeRead(address,size));
	}

	public Result _run(byte[] md5, int[] arg) {
		return exchange( MessageFactory.makeRun(md5, arg));
	}
	
	public Result _load(byte[] code) {
		return exchange( MessageFactory.makeLoad(code));
	}

	public Result _readbytes(int address, int count) {
		return exchange( MessageFactory.makeReadBytes(address, count));
	}

	public Result _writebytes(int address, byte[] data) {
		return exchange( MessageFactory.makeWriteBytes(address, data));
	}


	
	/*
	 * Communication stuff
	 */
	
	protected Result exchange(MessageFactory.Operation op){
		Result result = new Result();
		try {
			DatagramSocket socket = new DatagramSocket();
			
			ByteArrayOutputStream b = new ByteArrayOutputStream();
			DataOutputStream out = new DataOutputStream( b );
			
			op.write(out);
			out.close();
			byte[] ar = b.toByteArray();
			//over size packets get rejected
			if( ar.length > socket.getSendBufferSize() )
				throw new RuntimeException("Too large:"+
						ar.length +" max:"+socket.getSendBufferSize() );
			
			
			DatagramPacket p = new DatagramPacket(ar, ar.length);

			p.setAddress(server);
			p.setPort(ActiveRDMA.SERVER_PORT);
			
			socket.send(p);
			
			int n_retry = 3;
			while( n_retry-- > 0 ){
				try{
				socket.setSoTimeout(ActiveRDMA.REQUEST_TIMEOUT);
				socket.receive(p);
				break;
				}catch(SocketTimeoutException e){
					//re send
					socket.send(p);
				}
			}
			
			if( n_retry == -1 ){
				result = new Result(ErrorCode.TIME_OUT);
				return result;
			}
			
			DataInputStream in = new DataInputStream(new ByteArrayInputStream(p.getData()));
			result.read(in);

			socket.close();
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		return result;
	}

}

/* OLD TCP Client

protected int tcp_exchange(MessageFactory.Operation op){
	int result = 0;
	try {
		Socket socket = new Socket(server, ActiveRDMA.SERVER_PORT);
		
		DataOutputStream out = new DataOutputStream(socket.getOutputStream());
		op.write(out);
		out.flush();
		
		//always just one int returned
		DataInputStream in = new DataInputStream(socket.getInputStream());
		result = in.readInt();
		
		out.close();
		in.close();
		socket.close();
	} catch (UnknownHostException e) {
		e.printStackTrace();
	} catch (IOException e) {
		e.printStackTrace();
	}
	return result;
}

*/
