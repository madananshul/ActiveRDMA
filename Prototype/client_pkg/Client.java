
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

package client_pkg;

import java.net.*;
import java.io.*;

public class Client
{  private Socket socket              = null;
   private DataInputStream  console   = null;
   private DataOutputStream streamOut = null;
   private File myFile = null;
   private FileInputStream fis = null;
   private BufferedInputStream bis = null;
   private OutputStream os = null;

   public Client(String serverName, int serverPort)
   {  System.out.println("Establishing connection. Please wait ...");
      try
      {  socket = new Socket(serverName, serverPort);
         System.out.println("Connected: " + socket);
         start();
      }
      catch(UnknownHostException uhe)
      {  System.out.println("Host unknown: " + uhe.getMessage());
      }
      catch(IOException ioe)
      {  
    	  System.out.println("Unexpected exception: " + ioe.getMessage());
      }
      
      byte [] mybytearray  = new byte [(int)myFile.length()];
      try {
          int read = bis.read(mybytearray,0,mybytearray.length);
          System.out.println("Read " + read + "bytes from file.");
      }
      catch(IOException ioe)
      {
    	  System.out.println("Sending error: " + ioe.getMessage());
      }
      System.out.println("Sending file...");
      try {
    	  os.write(mybytearray,0,mybytearray.length);
          os.flush();
      }
      catch(IOException ioe)
      {
    	  System.out.println("Sending error: " + ioe.getMessage());
      }
      
      
   }
   public void start() throws IOException
   {  console   = new DataInputStream(System.in);
      streamOut = new DataOutputStream(socket.getOutputStream());
      
      myFile = new File ("Ins.class");
      //fis = new FileInputStream(myFile);
      try {
    	  bis = new BufferedInputStream(new FileInputStream(myFile));
      }
      catch(IOException ioe)
      {
    	  System.out.println("Sending error: " + ioe.getMessage());
      }
      try {
    	  os = socket.getOutputStream();
      }
      catch(IOException ioe)
      {
    	  System.out.println("Sending error: " + ioe.getMessage());
      }
      
           
   }
   public void stop()
   {  try
      {  if (console   != null)  console.close();
         if (streamOut != null)  streamOut.close();
         if (socket    != null)  socket.close();
      }
      catch(IOException ioe)
      {  System.out.println("Error closing ...");
      }
   }
   public static void main(String args[])
   {  Client client = null;
      if (args.length != 2)
         System.out.println("Usage: java Client host port");
      else{
         client = new Client(args[0], Integer.parseInt(args[1]));
      }
   }
}