package examples.dht;

import common.*;

//import client.Client;

// hash table is at address N, has N entries, each entry is 
// <next> <data> <keylen> <key...>
//
// word 0 is the free-memory pointer (trivial malloc with no free)

public class DHT_RDMA implements DHT {

	private ActiveRDMA m_client;
	private int N;
	
	public DHT_RDMA(ActiveRDMA client, int _N)
	{
		m_client = client;
		N = _N;
		
		// initialize the free-block pointer
		m_client.w(0, 2*N);
		
		// initialize the hash entries
		for (int i = 0; i < N; i++)
			m_client.w(4*N+i, 0);
	}
	
	int hash(byte[] key)
	{
		int t = 0;
		for (int i = 0; i < key.length; i++) t = 37*t + key[i];
		return t % N;
	}
	
	boolean compareKey(byte[] key, int ptr)
	{
        int len = m_client.r(ptr + 8);
        if (len != key.length) return false;

        byte[] b = m_client.readbytes(ptr + 12, key.length);
        for (int i = 0; i < key.length; i++)
            if (key[i] != b[i])
                return false;

        return true;
	}
	
	int findKey(byte[] key)
	{
		int ptr = m_client.r(4 * (N + hash(key)));

		while (ptr != 0)
		{
			if (compareKey(key, ptr)) break;
			
			ptr = m_client.r(ptr);
		}
		
		return ptr;
	}
	
	public int get(String key)
	{
		int ptr = findKey(key.getBytes());
		return (ptr != 0) ? m_client.r(ptr + 4) : 0;
	}
	
	public void put(String key, int val)
	{
		int newPtr;
        byte[] k = key.getBytes();
        int recLen = k.length + 12;
        recLen = (recLen + 3) & ~4; // round up to next word
		
		// create the new data block first
		while (true)
		{
			newPtr = m_client.r(0);
			if (m_client.cas(0, newPtr, newPtr + recLen) != 0) break;
		}

		m_client.w(newPtr, 0);
		m_client.w(newPtr + 4, val);
        m_client.w(newPtr + 8, k.length);
        m_client.writebytes(newPtr + 12, k);
		
        int hashPtr = 4*(N+hash(k));
		while (true)
		{
			int oldHead = m_client.r(hashPtr);
			m_client.w(newPtr, oldHead);
			if (m_client.cas(hashPtr, oldHead, newPtr) != 0) break;
		}
	}

	public boolean has(String key) {
        byte[] k = key.getBytes();
        return findKey(k) != 0;
	}
}
