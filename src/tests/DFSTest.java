package tests;

import junit.framework.TestCase;
import client.Client;
import common.ActiveRDMA;
import dfs.DFS;
import dfs.DFS_RDMA;

public class DFSTest extends TestCase {

    final String server = "localhost";


    public void tests_DFS_RDMA() throws Exception {
        ActiveRDMA client = new Client(server);

        DFS dfs;
        dfs = new DFS_RDMA(client);
        int[] inodes = new int[10];
        for(int i = 0;i<10;i++){
        	inodes[i] = dfs.create("file_" + i);
        	System.out.println("dfs create : "+ "file_" + i + " inode "+ inodes[i]);
        }
        for(int i = 0;i<10;i++){
        	int temp = dfs.lookup("file_" + i);
        	System.out.println("dfs lookup : "+ "file_" + i + " inode " + temp);
        	assertEquals(inodes[i],  temp);
        }
        
    }

}