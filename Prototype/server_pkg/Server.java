
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

import java.net.*;
import java.io.*;

import java.net.*;
import java.io.*;

public class Server implements Runnable
{  private Socket       socket = null;
   private ServerSocket server = null;
   private Thread       thread = null;
   private DataInputStream  streamIn  =  null;
   private InputStream is = null;
   private FileOutputStream fos = null;
   private BufferedOutputStream bos = null;
   static byte[] memory= new byte[100000] ;
   

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
            byte [] mybytearray  = new byte [filesize];
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
            
            executeInstructions();
            
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
      fos = new FileOutputStream("Instructions.class");
      bos = new BufferedOutputStream(fos);
   }
   
   public void executeInstructions(){
	   Instructions ins=new Instructions();
	   /*this code should be automatically be inserted by server into byte code and does not 
		 * have to be written by client program
		 */
		int instructions_length = ins.Instruction_set.length;//compute from no of instructions specified
		byte[] mem = Server.memory;
		byte results[] = new byte[instructions_length];
		boolean bool [] = new boolean[instructions_length];
	
		/*END*/
		
		String Instruction_set[] = ins.Instruction_set;
		for(int i=0;i<ins.Instruction_set.length;i++){
			if(ins.Instruction_set[i].equalsIgnoreCase("Read")){
				results[i] = mem[ins.first_args[i]];
			}
			else if(ins.Instruction_set[i].equalsIgnoreCase("Write")){		
				results[i] = mem[ins.first_args[i]];
				mem[ins.first_args[i]]=ins.second_args[i];
			}
			else if(ins.Instruction_set[i].equalsIgnoreCase("CAS")){
				results[i] = mem[ins.first_args[i]];
				mem[ins.first_args[i]]=ins.second_args[i];
				if(mem[ins.first_args[i]]==ins.second_args[i]){
					results[i] = mem[ins.first_args[i]];
					mem[ins.first_args[i]]=ins.third_args[i];
				}
				else{
					results[i] = -1;
				}
			}
			System.out.println("Contents of memory at offset "+ins.first_args[i]+" is "+mem[ins.first_args[i]]);
		}
		//return results[] to client
	   
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
