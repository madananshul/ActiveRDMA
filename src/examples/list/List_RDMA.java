package examples.list;

import common.ExtActiveRDMA;

public class List_RDMA implements List {

	protected final ExtActiveRDMA c;
	protected final int root_ptr;

	static final int NULL = -1;

	public List_RDMA(ExtActiveRDMA c){
		this.c = c;
		this.root_ptr = c.alloc(2);

		//slot 0 is never used
		c.w(root_ptr+1, NULL);
	}

	public int add(int value) {
		//new node
		int node = c.alloc(2);
		c.w(node, value); // node[0] = content
		c.w(node+1, NULL);  // node[1] = link

		//now try to put it at the end
		int pos = root_ptr;
		while( c.cas(pos+1,NULL,node) == 0 ){
			//link was not NULL
			//follow the link of the node
			pos = c.r(pos+1);
		}

		return node;
	}

	public int get(int n) {
		int pos = c.r(root_ptr+1);

		if( pos==NULL )
			return NULL;
		
		while( n-- > 0 && pos!=NULL ){
			pos = c.r( pos+1 );
		}

		return pos!=NULL ? c.r(pos) : NULL;
	}

}
