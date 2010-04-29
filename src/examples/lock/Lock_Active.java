package examples.lock;

import common.ActiveRDMA;

public class Lock_Active implements examples.lock.Lock {

	static final int N = 1024;
	static final int NULL = -1;
	protected ActiveRDMA c;

	public static class Lock_Create {		
		//creating lock table
		public static int execute(ActiveRDMA c, int[] args) {
			int size = args[0];
			int head_of_table = c.alloc(4 * 2*size);
			for(int i=0;i<8*size;i++)
				c.w(head_of_table+i, NULL);
			return head_of_table;
		}
	}
	
	
	/*
	 * P releases a lock: Checks p->next:
	 * If not NIL, remote write on next node's locked variable to set to false
	 * (releasing the lock). Free its own node ??
	 * Otherwise: CAS(NIL, p) op on lock variable. If this fails, it means another 
	 * process began requesting lock, and has updated lock variable, but has not yet
	 * updated lock p's next variable. Process p then polls on its next variable
	 * until set by other process, then set locked of other process to false.
	 */
	public static class Lock_Release {		
		
		public static int execute(ActiveRDMA c, int[] args) {
			int node = args[0], lock = args[1];

            System.out.println("Starting release, lock is " + lock);
			
			int next = c.r(node+4);
			if(next != NULL){
				c.w(next, 0);
			}
			else {
				if(c.cas(lock, node, NULL) != 0){
					//free node
				}
				else {
					//poll on next until it is set 
					while( (next = c.r(node+4)) == NULL ){
						//next = NULL
					}
					c.w(next, 0);
				}
			}
			System.out.println("Released lock_id :"+node);
			return 0;
		}
	}


	/* Process p requests lock : 
	 * node : locked(int), next(int)
	 * Sets its node next to NIL, fetch&write(p) on the lock variable.
	 * If predecessor is NIL: it acquires lock, lock points to it.
	 * Else: sets p->locked = true, remote write to write(p) to predecessor's next 
	 * variable. the polls on its locked variable until it becomes false.(can this 
	 * polling be bettered so that there are not so many network round trips.)
	*/
	public static class Lock_Lock {
				
		public static int execute(ActiveRDMA c, int[] args) {
			int file = args[0], lock = args[1];
			//new node
			int node = c.alloc(4 * 2);
			//System.out.println("New node for lock is at address :"+node);
			
			c.w(node+4, NULL);  // node[1] = link
	
			int predecessor;
			
			predecessor = c.r(lock+4);
			while(c.cas(lock+1, predecessor, node)==0){
				predecessor = c.r(lock+4);
			}
			if(predecessor == NULL){
				c.w(node, 0);
			}
			else {
				c.cas(predecessor+4, NULL, node);
				c.w(node, 4);
			}
			
			//poll on locked until it is false
			while( c.r(node) != 0 ){
				//node->locked was true
			}
			//Now node->locked = false, meaning lock has been granted
			System.out.println("Granted lock on file:"+file+" , lock_id is "+ node);
			return node;
		}
	}

	public Lock_Active(ActiveRDMA c) {
		this.c = c;

		c.load(Lock_Create.class);
		c.load(Lock_Lock.class);
		c.load(Lock_Release.class);
		
	}

	public int create(int size) {
		int[] k = {size};

		return c.run(Lock_Create.class, k);
	}
	
	public int lock(int file, int table) {
		int[] k = {file, table};

		return c.run(Lock_Lock.class, k);
	}
	
	
	public int release(int node, int table) {
		int[] k = {node, table};

		return c.run(Lock_Release.class, k);
	}

}
