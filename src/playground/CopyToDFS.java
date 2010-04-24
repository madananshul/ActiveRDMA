package playground;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;

import client.Client;
import dfs.DFS;
import dfs.DFS_Active;

public class CopyToDFS {

	/** copies file into DFS
	 * @param file - name of local file to be copied into DFS
	 */
	public static void copy(DFS dfs, String file) throws Exception {
		int inode = dfs.create(file);
		File f = new File(file);
		dfs.setLen(inode, (int)f.length());
		
		InputStream in = new FileInputStream(f);
		byte[] buffer = new byte[256];
		int offset = 0;
		int len = 0;
		while( (len = in.read(buffer)) != -1)
        {
			dfs.put(inode, buffer, offset, len);
            offset += len;
        }
		in.close();
	}
	
	/** prints DFS file into System.out
	 * @param file
	 */
	public static void print(DFS dfs, String file) throws Exception {
		byte[] buffer = new byte[512];
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
	}
	
	public static void main(String[] args) throws Exception{
        DFS dfs = new DFS_Active( new Client("localhost") );
		copy(dfs, "src/playground/test");
		System.out.println("copy completed.");
		System.out.println("----------------");
		print(dfs, "src/playground/test");
		System.out.println("");
		System.out.println("----------------");
		System.out.println("print completed.");
	}
	
//	 final String server = "localhost";
//
//
//	    public void tests_DFS_RDMA() throws Exception {
//	        ActiveRDMA client = new Client(server);
//
//	        DFS dfs;
//	        dfs = new DFS_Active(client);
//	        int[] inodes = new int[10];
//	        for(int i = 0;i<10;i++){
//	        	inodes[i] = dfs.create("file_" + i);
//	        	System.out.println("dfs create : "+ "file_" + i + " inode "+ inodes[i]);
//	        }
//	        for(int i = 0;i<10;i++){
//	        	int temp = dfs.lookup("file_" + i);
//	        	System.out.println("dfs lookup : "+ "file_" + i + " inode " + temp);
//	        	assertEquals(inodes[i],  temp);
//	        }
//
//	        int inode = dfs.create("asdf");
//
//	        byte[] buf = new byte[1024];
//	        for (int i = 0; i < 1024; i++) buf[i] = (byte)(i & 0xff);
//
//	        dfs.setLen(inode, 1048576); // 1 MB
//
//	        for (int i = 0; i < 32; i++)
//	            dfs.put(inode, buf, 1024*i, 1024);
//
//	        dfs.get(inode, buf, 4096 - 2, 4);
//	        for (int i = 0; i < 4; i++)
//	            assertEquals(buf[i], (byte)(((i + 1024 - 2) % 1024) & 0xff));
//
//	    }
	    
}
