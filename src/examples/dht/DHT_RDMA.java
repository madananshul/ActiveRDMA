package examples.dht;

import client.Client;

//import client.Client;

// hash table is at address N, has N entries, each entry is 
// <next> <data> <key0> <key1> ... 0.
//
// word 0 is the free-memory pointer (trivial malloc with no free)

public class DHT_RDMA implements DHT {

	private Client m_client;
	private int N;
	
	public DHT_RDMA(Client client, int _N)
	{
		m_client = client;
		N = _N;
		
		// initialize the free-block pointer
		m_client.w(0, 2*N);
		
		// initialize the hash entries
		for (int i = 0; i < N; i++)
			m_client.w(N+i, 0);
	}
	
	int hash(int[] key)
	{
		int t = 0;
		for (int i = 0; i < key.length; i++) t ^= key[i];
		return t % N;
	}
	
	int[] stringToInt(String s)
	{
		byte[] bytes = s.getBytes();
		int[] ret = new int[(s.length()+2)/4];
		for (int i = 0; i < ret.length; i++) ret[i] = 0;
		
		for (int i = 0; i < s.length(); i++)
		{
			ret[i/4] <<= 8;
			ret[i/4] |= bytes[i];
		}
		
		return ret;
	}
	
	boolean compareKey(int[] key, int ptr)
	{
		for (int i = 0; i < key.length; i++)
		{
			int val = m_client.r(ptr + i);
			if (val != key[i]) return false;
		}
		
		return m_client.r(ptr + key.length) == 0;
	}
	
	int findKey(int[] key)
	{
		int ptr = m_client.r(N + hash(key));
		
		while (ptr != 0)
		{
			if (compareKey(key, ptr + 2)) break;
			
			ptr = m_client.r(ptr);
		}
		
		return ptr;
	}
	
	public int get(String key)
	{
		int[] k = stringToInt(key);
		int ptr = findKey(k);
		return (ptr != 0) ? m_client.r(ptr + 1) : 0;
	}
	
	public void put(String key, int val)
	{
		int[] k = stringToInt(key);
		int newPtr;
		
		// create the new data block first
		while (true)
		{
			newPtr = m_client.r(0);
			if (m_client.cas(0, newPtr, newPtr + k.length + 2) != 0) break;
		}
		
		m_client.w(newPtr, 0);
		m_client.w(newPtr + 1, val);
		for (int i = 0; i < k.length; i++)
			m_client.w(newPtr + 2 + i, k[i]);
		
		while (true)
		{
			int oldHead = m_client.r(N + hash(k));
			m_client.w(newPtr + 1, oldHead);
			if (m_client.cas(N + hash(k), oldHead, newPtr) != 0) break;
		}
	}

	public boolean has(String key) {
		// TODO Auto-generated method stub
		return false;
	}
	
}
