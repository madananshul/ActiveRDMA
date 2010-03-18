package examples.list;

public interface List {

	/**
	 * @param value - the new value
	 * @return fail (0) or success (!0)
	 */
	int add(int value);
	
	int get(int n_elem);
	
}
