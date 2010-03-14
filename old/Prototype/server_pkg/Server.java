
/*
 * Here are the basic operations:

R(Addr) -> Int
W(Addr, Int) -> Int // returns old value
CAS(Addr, Int, Int) -> Int // compares Mem[Addr] to 2nd arg; if equal,
swaps 3rd arg in; returns old val
RUN(Addr, Int) -> Void // loads class at Address, runs invoke() as defined below

The signature for a class is:

class <name_doesnt_matter> {

public static void invoke(int arg, int[] memory);

}

The wire protocol should have these operations:

Execute(Insn[]) -> Results, Bool[] // execute operations; results is
an array of the return values from each instruction (specified above),
and success/failure bools for each insn
 */

package server_pkg;

import java.net.*;
import java.io.*;
import client_pkg.*;
import java.lang.ClassLoader.*;

public class Server extends Server_interface_class implements Runnable
{  private Socket       socket = null;
   private ServerSocket server = null;
   private Thread       thread = null;
   private DataInputStream  streamIn  =  null;
   private InputStream is = null;
   private FileOutputStream fos = null;
   private BufferedOutputStream bos = null;
   static byte[] memory= new byte[100000] ;
   
   public byte[] getMemory(){
	   return memory;
   }
   

   public Server(int port)
   {  try
      {  System.out.println("Binding to port " + port + ", please wait  ...");
         server = new ServerSocket(port);  
         System.out.println("Server started: " + server);
         start();
      }
      catch(IOException ioe)
      {  System.out.println(ioe); 
      }
   }
   
   public Server()
   {  
   }
   
   public void run()
   {  while (thread != null)
      {   try
         {  
    	    int filesize=6022386; // filesize temporary hardcoded

    	    long start = System.currentTimeMillis();
    	    int bytesRead;
    	    int current = 0;

    	    System.out.println("Waiting for a client ..."); 
            socket = server.accept();
            System.out.println("Client accepted: " + socket);
            open();
            int commandbytearraylength = 0;
            byte [] commandbytearray  = new byte [100];
            byte [] mybytearray  = new byte [filesize];
            
            //First 100 bytes sent are just the bytes corresponding to the 
            //command(including padding)
            ////First 'int' bytes are length of command
            commandbytearraylength=is.read();//assuming int is 4 bytes
            System.out.println("Server Received lenght " +commandbytearraylength+"command");
            bytesRead = is.read(commandbytearray,0,commandbytearraylength);
            String command = new String(commandbytearray, 0, commandbytearraylength);
            System.out.println("Server Received command  "+command);
            
            if(command.equalsIgnoreCase("PutCode")){
            	//TBD - Add code that returns refrence to code file back to client
            	bytesRead = is.read(mybytearray,0,mybytearray.length);
            	current = bytesRead;
            	System.out.println("after reading stream from socket , bytes : " + bytesRead);
            	/*do {
               	bytesRead =
                  	is.read(mybytearray, current, (mybytearray.length-current));
               	if(bytesRead >= 0) current += bytesRead;
            	} while(bytesRead > -1);*/

            	System.out.println("Writing class file...");
            	bos.write(mybytearray, 0 , current);
            	bos.flush();
            	long end = System.currentTimeMillis();
            	System.out.println(end-start);
            }
            
            else if(command.equalsIgnoreCase("ExecuteCode")){
            	//TBD - Add ocde that takes in reference to Code file as input from CLient
            	executeInstructions();
            	System.out.println("Contents of memory at offset "+0+" after setting to 1 is "+Server.memory[0]);
            }
            else if(command.equalsIgnoreCase("Read")){
            	//Do this op on Server.memory
            	int addr=is.read();//assuming int is 4 bytes
            	byte results = Server.memory[addr];
            }
            else if(command.equalsIgnoreCase("Write")){
            	//DO this op on Server.memory
            	int addr=is.read();//assuming int is 4 bytes
            	byte results = Server.memory[addr];
            	byte second_args=(byte)is.read();//assuming int is 4 bytes
            	Server.memory[addr] = second_args;
            }
            else if(command.equalsIgnoreCase("CAS")){
            	//DO this op on Server.memory
            	int addr=is.read();//assuming int is 4 bytes
            	int results = Server.memory[addr];
            	byte second_args=(byte)is.read();//assuming int is 4 bytes
            	byte third_args=(byte)is.read();//assuming int is 4 bytes
            	Server.memory[addr]=second_args;
       		 	if(Server.memory[addr]==second_args){
       		 		results = Server.memory[addr];
       		 		Server.memory[addr]=third_args;
       		 	}
            }
            
            close();
         }
         catch(IOException ie)
         {  //System.out.println("Acceptance Error: " + ie);  
         }
      }
   }
   public void start()
   {  if (thread == null)
      {  thread = new Thread(this); 
         thread.start();
      }
   }
   public void stop()
   {  if (thread != null)
      {  thread.stop(); 
         thread = null;
      }
   }
   public void open() throws IOException
   {  
      is = socket.getInputStream();
      fos = new FileOutputStream("./Prototype/client_pkg/Instructions.class");
      bos = new BufferedOutputStream(fos);
   }
   
   public void executeInstructions(){

	    //TBD - Add ocde that takes in reference to Code file as input from CLient
	    Class aClass=null;
	    Instructions_interface_class ins= null;
	    try {
	    	aClass = Class.forName("client_pkg.Instructions");
	    } catch (ClassNotFoundException e) {
	        e.printStackTrace();
	    }
	    try {
	    	if(aClass!=null)System.out.println("Class was instantiated");
	    	//if(aClass!=null)ins = aClass.newInstance();
	    	if(aClass!=null)ins = (Instructions_interface_class)aClass.newInstance();
	    } catch (Exception e) {
	        e.printStackTrace();
	    }

	    //byte[] mem = Server.memory;
	    Server.memory = ins.execute();
		
		//return 0;
	   
   }
   
   public void close() throws IOException
   {  
	  if (socket != null)    socket.close();
      if (streamIn != null)  streamIn.close();
      if(fos!=null) fos.close();
      if(bos!=null)bos.close();
   }
   public static void main(String args[])
   {  Server server = null;
      if (args.length != 1)
         System.out.println("Usage: java Server port");
      else
         server = new Server(Integer.parseInt(args[0]));
   }
}
