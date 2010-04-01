package examples.list;

import common.ActiveRDMA;

public class List_Active implements List{

	public static class List_AddNode{
		public static int execute(ActiveRDMA c, int[] args) {
			int value = args[0];
			int root_ptr = args[1];
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
	}
	
	public static class List_GetNode{
		public static int execute(ActiveRDMA c, int[] args) {
			int n = args[0];
			int root_ptr = args[1];
			
			int pos = c.r(root_ptr+1);

			if( pos==NULL )
				return NULL;
			
			while( n-- > 0 && pos!=NULL ){
				pos = c.r( pos+1 );
			}

			return pos!=NULL ? c.r(pos) : NULL;
		}
	}
	
	public static int makeRoot(ActiveRDMA c){
		int root = c.alloc(2);
		//slot 0 never used
		c.w(root+1, NULL);
		return root;
	}
	
	protected final ActiveRDMA c;
	protected final int root_ptr;
	
	static final int NULL = -1;
	
	public List_Active(ActiveRDMA c){
		this.c = c;
		this.root_ptr = makeRoot(c);
		
		//ignores result!
		c._load(List_AddNode.class);
		c._load(List_GetNode.class);
	}
	
	public int add(int value) {
		return c.run(List_AddNode.class, new int[]{value,root_ptr});
	}

	public int get(int n) {
		return c.run(List_GetNode.class, new int[]{n,root_ptr});
	}

}
