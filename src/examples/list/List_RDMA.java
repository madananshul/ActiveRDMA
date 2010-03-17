package examples.list;

import common.ExtActiveRDMA;

public class List_RDMA implements List {

	protected final ExtActiveRDMA c;
	protected final int root_ptr;
	
	static final int NULL = -1;
	
	public List_RDMA(ExtActiveRDMA c, int root_ptr){
		this.c = c;
		this.root_ptr = root_ptr;
		
		c.w(root_ptr, NULL);
	}
	
	public int add(int value) {
		//new node
		int node = c.alloc(2);
		c.w(node, value); // node[0] = content
		c.w(node, NULL);  // node[1] = link
		
		//now try to put it at the end
		int pos = root_ptr;
		while( c.cas(pos,NULL,node) == 0 ){
			//link was not null
			//follow the link of the node
			pos = c.r( c.r(pos)+1 );
		}
		
		return node;
	}

}
