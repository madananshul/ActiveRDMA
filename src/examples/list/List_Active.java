package examples.list;

import java.util.concurrent.atomic.AtomicInteger;

import common.ExtActiveRDMA;

public class List_Active implements List{

	public static class List_Active_AddNode{
		public static int execute(AtomicInteger[] mem, int[] args) {
			//TODO: hidden links to other classes...
			int pos = args[1];
			while( !mem[pos].compareAndSet(NULL, args[0]) ){
				//link was not NULL
				//follow the link of the node
				pos = mem[ mem[pos].get()+1 ].get();
			}
			return args[0];
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
		
		return c.run(List_Active_AddNode.class, new int[]{node,root_ptr});
	}

}
