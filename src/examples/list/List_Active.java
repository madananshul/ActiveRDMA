package examples.list;

import java.util.concurrent.atomic.AtomicInteger;

import common.ExtActiveRDMA;

public class List_Active implements List{

	public static class List_Active_AddNode{
		public static int execute(AtomicInteger[] mem, int[] args) {
			int pos = args[1];
			while( !mem[pos+1].compareAndSet(NULL, args[0]) ){
				//link was not NULL
				//follow the link of the node
				pos = mem[pos+1].get();
			}
			return args[0];
		}
	}
	
	public static class List_Active_GetNode{
		public static int execute(AtomicInteger[] mem, int[] args) {
			int pos = mem[args[1]+1].get();
			
			if( pos==NULL )
				return NULL;
			
			while( args[0]-- > 0 && pos!=NULL ){
				pos = mem[pos+1].get();
			}
			
			return pos!=NULL ? mem[pos].get() : NULL;
		}
	}
	
	protected final ExtActiveRDMA c;
	protected final int root_ptr;
	
	static final int NULL = -1;
	
	public List_Active(ExtActiveRDMA c){
		this.c = c;
		this.root_ptr = c.alloc(2);
		
		c.load(List_Active_AddNode.class);
		c.load(List_Active_GetNode.class);
		
		//slot 0 never used
		c.w(root_ptr+1, NULL);
	}
	
	public int add(int value) {
		//new node
		int node = c.alloc(2);
		c.w(node, value); // node[0] = content
		c.w(node+1, NULL);  // node[1] = link
		
		return c.run(List_Active_AddNode.class, new int[]{node,root_ptr});
	}

	public int get(int n) {
		return c.run(List_Active_GetNode.class, new int[]{n,root_ptr});
	}

}
