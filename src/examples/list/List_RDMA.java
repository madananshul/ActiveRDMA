package examples.list;

import common.ActiveRDMA;
import examples.list.List_Active.List_AddNode;
import examples.list.List_Active.List_GetNode;

public class List_RDMA implements List {

	protected final ActiveRDMA c;
	protected final int root_ptr;

	static final int NULL = -1;

	public List_RDMA(ActiveRDMA c){
		this.c = c;
		this.root_ptr = List_Active.makeRoot(c);
	}

	public int add(int value) {
		//note that this runs locally
		return List_AddNode.execute(c, new int[]{value,root_ptr});
	}

	public int get(int n) {
		//note that this runs locally
		return List_GetNode.execute(c, new int[]{n,root_ptr});
	}

}
