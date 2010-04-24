package playground;

import java.util.Scanner;

import client.Client;
import dfs.DFS;
import dfs.DFS_Active;

public class Grep {

	public static void grep(String file, String pattern, DFS server) throws Exception {
		Scanner sc = new Scanner(new DFSInputStream(file, server)
		);
		while( sc.hasNextLine() ){
			String line = sc.nextLine();
			if( line.matches(pattern) )
				System.out.println(line);
		}
		sc.close();
	}
	
	/*
	 *
.begin
apple
apple-
apple-fruit
fruit-apple
.done
	 */
	public static void main(String[] args) throws Exception{
		DFS dfs = new DFS_Active( new Client("localhost") );
		
		System.out.println(".begin");
		grep("src/playground/test","(.*\\W)?apple(\\W.*)?",dfs);
		System.out.println(".done");
	}


}
