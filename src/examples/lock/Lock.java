package examples.lock;

public interface Lock {
		
	int create(int size);
	int lock(int file, int table);
	int release(int node, int table);
	
}
