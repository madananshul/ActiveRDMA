package examples.list;

import java.util.concurrent.atomic.AtomicInteger;

import common.ExtActiveRDMA;

public class List_Active implements List{

	//FIXME: THIS WILL CRASH! NO ACTUAL ROOT POINTER
	public static class List_Active_AddNode{
		public static int execute(AtomicInteger[] mem, int node) {
			int pos = -1; //FIXME: SHIT!! root_ptr; FIXME: improve interface? more arguments?
			while( !mem[pos].compareAndSet(NULL, node) ){
				//link was not NULL
				//follow the link of the node
				pos = mem[ mem[pos].get()+1 ].get();
			}
			return node;
		}
	}
	
	protected final ExtActiveRDMA c;
	protected final int root_ptr;
	
	static final int NULL = -1;
	
	public List_Active(ExtActiveRDMA c, int root_ptr){
		this.c = c;
		this.root_ptr = root_ptr;
		
		c.load(List_Active_AddNode.class);
		c.w(root_ptr, NULL);
	}
	
	public int add(int value) {
		//new node
		int node = c.alloc(2);
		c.w(node, value); // node[0] = content
		c.w(node, NULL);  // node[1] = link
		
		return c.run(List_Active_AddNode.class, node);
	}

}
