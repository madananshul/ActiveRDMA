package fsutils;

import java.io.IOException;
import java.util.Scanner;

import client.Client;

import playground.DFSInputStream;

import common.ActiveRDMA;

import dfs.DFS;
import dfs.DFS_RDMA;

public class Grep {

	public static class Active_Grep
	{

		//inode
		//pattern
		public static int[] execute(ActiveRDMA c, int[] args)
		{
			DFS dfs = new DFS_RDMA(c, true);
			int inode = args[0];
			String pattern = ActiveRDMA.getString(args, 1);
			String res = ""; //System.err.println(pattern); // (.*\W)?apple(\W.*)?

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

	//
	//
	//

	public static void main(String[] args)
	{
		if (args.length < 4)
		{
			System.err.println("Usage: Grep <hostname> <active | rdma> <pattern> <file>");
			return;
		}

		Client c;
		try {
			c = new Client(args[0]);
		} catch (IOException e) {
			System.err.println(e.toString());
			return;
		}

		DFS dfs = new DFS_RDMA(c, true);

		boolean active = args[1].equals("active");
		String pattern = args[2];
		String file = args[3];

		int[] arg = ActiveRDMA.constructArgs(1, pattern);
		arg[0] = dfs.lookup(file);
		//FIXME: assumes file exists!

		int[] results;
		
		if (active) {
			c.load(Active_Grep.class);
			results = c.runArray(Active_Grep.class, arg);
		}
		else
			results = Active_Grep.execute(c, arg);
		
		System.out.println( ActiveRDMA.getString(results, 0) );
	}

}
