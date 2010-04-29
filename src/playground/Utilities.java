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

	static class Grep{
		public static void execute(ActiveRDMA c, int[] args) throws Exception {
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
		}
	}

	static class FileCopy{
		static final int BUF_SIZE = 512;

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

	static class CopyToDFS{
		static final int BUF_SIZE = 512;

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

	static class PrintFile{
		static final int BUF_SIZE = 512;

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

	static class PrintInode{
		static final int BUF_SIZE = 512;

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


	public static void main(String[] args) throws Exception{

		ActiveRDMA c = new Client("localhost");
		Utilities.CopyToDFS.execute(c, ActiveRDMA.constructArgs(0,"src/playground/test") );
		System.out.println("copy completed.");
		System.out.println("----------------");

		Utilities.PrintFile.execute(c, ActiveRDMA.constructArgs(0,"src/playground/test") );
		System.out.println("");
		System.out.println("----------------");
		System.out.println("print completed.");

		
//		DFS dfs = new DFS_Active( new Client("localhost") );
//		
//		System.out.println(".begin");
//		grep("src/playground/test","(.*\\W)?apple(\\W.*)?",dfs);
//		System.out.println(".done");
	}
	
}
