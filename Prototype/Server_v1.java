
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
         {  System.out.println("Waiting for a client ..."); 
            socket = server.accept();
            System.out.println("Client accepted: " + socket);
            open();
            boolean done = false;
            while (!done)
            {  try
               {  String line = streamIn.readUTF();
                  System.out.println(line);
                  done = line.equals(".bye");
               }
               catch(IOException ioe)
               {  done = true;  }
            }
            close();
         }
         catch(IOException ie)
         {  System.out.println("Acceptance Error: " + ie);  }
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
   {  streamIn = new DataInputStream(new 
                        BufferedInputStream(socket.getInputStream()));
   }
   public void close() throws IOException
   {  if (socket != null)    socket.close();
      if (streamIn != null)  streamIn.close();
   }
   public static void main(String args[])
   {  Server server = null;
      if (args.length != 1)
         System.out.println("Usage: java Server port");
      else
         server = new Server(Integer.parseInt(args[0]));
   }
}
