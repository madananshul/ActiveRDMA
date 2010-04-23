package tests;

import junit.framework.TestCase;
import client.Client;
import common.ActiveRDMA;
import dfs.DFS;
import dfs.DFS_RDMA;
import dfs.DFS_Active;

public class DFSTest extends TestCase {

    final String server = "localhost";


    public void tests_DFS_RDMA() throws Exception {
        ActiveRDMA client = new Client(server);

        DFS dfs;
        dfs = new DFS_Active(client);
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

        int inode = dfs.create("asdf");

        byte[] buf = new byte[1024];
        for (int i = 0; i < 1024; i++) buf[i] = (byte)(i & 0xff);

        dfs.setLen(inode, 1048576); // 1 MB

        for (int i = 0; i < 32; i++)
            dfs.put(inode, buf, 1024*i, 1024);

        dfs.get(inode, buf, 4096 - 2, 4);
        for (int i = 0; i < 4; i++)
            assertEquals(buf[i], (byte)(((i + 1024 - 2) % 1024) & 0xff));

    }
}
