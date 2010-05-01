package playground;

import java.io.IOException;
import java.io.InputStream;

import dfs.DFS;

public class DFSInputStream extends InputStream{

	final static int SIZE = 512;

	DFS dfs;
	int inode, len, off;
	byte[] buffer;
	
	public DFSInputStream(int inode, DFS dfs) {
		this.dfs = dfs;
		this.inode = inode;
		this.len = dfs.getLen(this.inode);
		this.buffer = new byte[SIZE];
	}

	@Override
	//-1 on EOF
	public int read() throws IOException {
		if( off >= len )
			return -1;
		dfs.get(inode, buffer, off, 1);
		off += 1;
		return buffer[0];
	}

	@Override
	//-1 on EOF
	public int read(byte b[], int o, int l) {
		if( off >= len )
			return -1;
		int n = Math.min(l, (len-off) );
		l = n;
		while( l>0 ){
			int read_block = Math.min(l, buffer.length);
			dfs.get(inode, buffer, off, read_block);
			off += read_block;
			System.arraycopy(buffer,0,b,o,read_block);
			o += read_block;
			l -= read_block;
		}
		return n;
	}

	@Override
	public void close() throws IOException { }

}