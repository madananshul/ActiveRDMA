



package client_pkg;

import server_pkg.*;

public class Instructions extends Instructions_interface_class{
	//public String Instruction_set[] = {"Read(0)", "Write(0, 5)", "CAS(0, 5, 7)"};
	public int no_of_instructions=3;
	public String Instruction_set[] = {"Read", "Write", "CAS"};
	public int first_args[] = {0, 0, 0};
	public byte second_args[] = {0, 5, 5};
	public byte third_args[] = {0, 0, 7};
	
	public  Instructions(){
		    //Server srv_obj = new Server();	
	}
	
	public byte[] execute(){
	    Class aClass=null;
	    Server_interface_class srv_obj= null;
	    try {
	    	aClass = Class.forName("server_pkg.Server");
	    } catch (ClassNotFoundException e) {
	        e.printStackTrace();
	    }
	    try {
	    	if(aClass!=null)System.out.println("Server Class was instantiated");
	    	
	    	if(aClass!=null)srv_obj = (Server_interface_class)aClass.newInstance();
	    } catch (Exception e) {
	        e.printStackTrace();
	    }
		byte[] mem = srv_obj.getMemory();
		System.out.println("Contents of memory at offset "+0+" before setting to 1 is "+mem[0]);
		for(int j=0;j<mem.length;j++)mem[j] = 1;
		return mem;
	}
	
}
