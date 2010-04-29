package playground;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Scanner;

import client.Client;

import common.ActiveRDMA;

import dfs.DFS;
import dfs.DFS_RDMA;

public class Utilities {

	public static class Grep{

		//inode
		//pattern
		public static int execute(ActiveRDMA c, int[] args) throws Exception {
			DFS dfs = new DFS_RDMA(c, true);
			int inode = args[0];
			String pattern = ActiveRDMA.getString(args, 1);

			Scanner sc = new Scanner(new DFSInputStream(inode, dfs) );
			while( sc.hasNextLine() ){
				String line = sc.nextLine();
				if( line.matches(pattern) )
					System.out.println(line);
			}
			sc.close();
			return 0;
		}
	}

	public static class GrepRes{

		//inode
		//pattern
		public static int[] execute(ActiveRDMA c, int[] args) throws Exception {
			DFS dfs = new DFS_RDMA(c, true);
			int inode = args[0];
			String pattern = ActiveRDMA.getString(args, 1);
			String res = "";
			
			Scanner sc = new Scanner(new DFSInputStream(inode, dfs) );
			while( sc.hasNextLine() ){
				String line = sc.nextLine();
				if( line.matches(pattern) )
					res += line+"\n";
			}
			sc.close();
			
			return ActiveRDMA.constructArgs(0, res);
		}
	}
	
	public static class FileCopy{
		static final int BUF_SIZE = 512;

		//from inode
		//to inode
		public static int execute(ActiveRDMA c, int[] args) throws Exception {
			DFS dfs = new DFS_RDMA(c, true);
			int from_inode = args[0];
			int to_inode = args[1];

			byte[] buffer = new byte[BUF_SIZE];
			int len = dfs.getLen(from_inode);

			dfs.setLen( to_inode, dfs.getLen(from_inode) );

			int offset = 0;
			while( len > 0 ){
				int read_length = Math.min(buffer.length, len);
				dfs.get( from_inode, buffer, offset, read_length );
				dfs.put( to_inode, buffer, offset, read_length );
				len -= read_length;
				offset += read_length;
			}
			return 0;
		}
	}

	public static class CopyToDFS{
		static final int BUF_SIZE = 512;

		//local file
		public static int execute(ActiveRDMA c, int[] args) throws Exception {
			DFS dfs = new DFS_RDMA(c, true);
			String file = ActiveRDMA.getString(args, 0);

			int inode = dfs.create(file);
			File f = new File(file);
			dfs.setLen(inode, (int)f.length());

			InputStream in = new FileInputStream(f);
			byte[] buffer = new byte[BUF_SIZE];
			int offset = 0;
			int len = 0;
			while( (len = in.read(buffer)) != -1)
			{
				dfs.put(inode, buffer, offset, len);
				offset += len;
			}
			in.close();

			return 0;
		}
	}

	public static class PrintFile{
		static final int BUF_SIZE = 512;

		// file
		public static int execute(ActiveRDMA c, int[] args) throws Exception {
			DFS dfs = new DFS_RDMA(c, true);
			String file = ActiveRDMA.getString(args, 0);

			byte[] buffer = new byte[BUF_SIZE];
			int inode = dfs.lookup(file);
			int len = dfs.getLen(inode);
			int offset = 0;
			while( len > 0 ){
				int read_length = Math.min(buffer.length, len);
				dfs.get(inode, buffer, offset, read_length);
				System.out.print(new String(buffer,0,read_length));
				len -= read_length;
				offset += read_length;
			}

			return 0;
		}
	}

	public static class PrintInode{
		static final int BUF_SIZE = 512;

		// inode
		public static int execute(ActiveRDMA c, int[] args) throws Exception {
			DFS dfs = new DFS_RDMA(c, true);
			int inode = args[0];

			byte[] buffer = new byte[BUF_SIZE];
			int len = dfs.getLen(inode);
			int offset = 0;
			while( len > 0 ){
				int read_length = Math.min(buffer.length, len);
				dfs.get(inode, buffer, offset, read_length);
				System.out.print(new String(buffer,0,read_length));
				len -= read_length;
				offset += read_length;
			}

			return 0;
		}
	}

	/*
	 * Tests
	 */

	public static void main(String[] _) throws Exception{
//		rdma();
		active();
	}
	
	public static void rdma() throws Exception{
		ActiveRDMA c = new Client("localhost");
		DFS dfs = new DFS_RDMA(c, true);
		int args[] = null;

		//copy to dfs
		args = ActiveRDMA.constructArgs(0,"src/playground/test");
		Utilities.CopyToDFS.execute(c, args );

		System.out.println("(copy)----------------");

		args = ActiveRDMA.constructArgs(0,"src/playground/test");
		Utilities.PrintFile.execute(c, args );

		System.out.println("");
		System.out.println("(print)----------------");

		args = ActiveRDMA.constructArgs(1, "(.*\\W)?apple(\\W.*)?");
		Utilities.Grep.execute(c, args );
		
		//
		int from = dfs.lookup("src/playground/test");
		int to = dfs.create("trash");
		Utilities.FileCopy.execute(c, new int[]{from,to});
		Utilities.PrintInode.execute(c, new int[]{to});

		System.out.println("(file copy)----------------");

		args = ActiveRDMA.constructArgs(1, "(.*\\W)?apple(\\W.*)?");
		args[0] = to;
		args = Utilities.GrepRes.execute(c, args );
		System.out.println( ActiveRDMA.getString(args, 0) );
		System.out.println("(grep)----------------");

	}
	
	public static void active() throws Exception{
		ActiveRDMA c = new Client("localhost");

		c._load( Grep.class );
		c._load( GrepRes.class );
		c._load( FileCopy.class );
		c._load( CopyToDFS.class );
		c._load( PrintFile.class );
		c._load( PrintInode.class );
		
		DFS dfs = new DFS_RDMA(c, true);
		int args[] = null;

		//copy to dfs USING SERVER's FS
		args = ActiveRDMA.constructArgs(0,"src/playground/test");
		c.run(CopyToDFS.class, args);
		
		//prints locally
		args = ActiveRDMA.constructArgs(0,"src/playground/test");
		Utilities.PrintFile.execute(c, args );

		int from = dfs.lookup("src/playground/test");
		int to = dfs.create("trash");
		
		c.run(FileCopy.class, new int[]{from,to});

		args = ActiveRDMA.constructArgs(1, "(.*\\W)?apple(\\W.*)?");
		args[0] = to;
		args = c.runArray( GrepRes.class, args );
		System.out.println("(grep)----------------");
		System.out.println( ActiveRDMA.getString(args, 0) );
		System.out.println("(grep)----------------");
	}

}
