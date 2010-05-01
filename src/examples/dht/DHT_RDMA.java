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

        init();
    }

    public DHT_RDMA(ActiveRDMA client, int _n, boolean noInit)
    {
        m_client = client;
        N = _n;

        if (m_client.r(0) < 4*2*N)
            init();
    }

    void init()
    {
        // initialize the free-block pointer
        m_client.w(0, 4* (2*N));

        // initialize the hash entries
        for (int i = 0; i < N; i++)
            m_client.w(4*(N+i), 0);
    }
	
	int hash(byte[] key)
	{
        // DJB hash
        int hash = 5381;
        for (int i = 0; i < key.length; i++)
            hash = ((hash << 5) + hash) + (((int)key[i])&0xff);
        return hash % N;
	}

    int hashPtr(byte[] key)
    {
        int h = hash(key);
        return 4*(N+h);
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
		int ptr = m_client.r(hashPtr(key));

		while (ptr != 0)
		{
			if (compareKey(key, ptr)) break;
			
			ptr = m_client.r(ptr);
		}
		
		return ptr;
	}
	
	int[] matchKey(int key, byte[] pattern_bytes)
	{
		//int ptr = m_client.r(hashPtr(key));
		int ptr;
		int[] ptr_array = new int[1000];
		int count = 0;
		String pattern = new String(pattern_bytes);
		
		for (int i = 0; i < N; i++){
			ptr = m_client.r(4*(N+i));
		
			while (ptr != 0)
			{
				int cur_key = m_client.r(ptr + 12);
			    int len = m_client.r(ptr + 8);
		        byte[] b = m_client.readbytes(ptr + 12, len);
		        int b_int[] = new int[b.length];
	            for (int j = 0; j < b_int.length; j++)
	                b_int[j] = (int)b[j];
		        
		        String cur_key_str = m_client.getString(b_int, 0);
				if( cur_key_str.matches(pattern) ) {
					ptr_array[count++] = ptr;
				}
				ptr = m_client.r(ptr);
			}
		}
		
		return ptr_array;
	}
	
	
	
    public int[] match(int key, String pattern)
    {
        return match(key, pattern.getBytes());
    }
	
	public int[] match(int key, byte[] pattern)
	{
		//TBD - return int array instead
		//boolean found = false;
		int ptr[] = matchKey(key, pattern);
		int[] ret_ptr = new int[ptr.length];
		
		for(int i=0;i<ptr.length;i++){
			ret_ptr[i] = (ptr[i] != 0) ? m_client.r(ptr[i] + 4) : 0;
		}
		
		return ret_ptr;
	}

    public int get(String key)
    {
        return get(key.getBytes());
    }
	
	public int get(byte[] key)
	{
		int ptr = findKey(key);
		return (ptr != 0) ? m_client.r(ptr + 4) : 0;
	}

    public void put(String key, int val)
    {
        put(key.getBytes(), val);
    }
	
	public void put(byte[] k, int val)
	{
		int newPtr;
        int recLen = k.length + 12;
        recLen = (recLen + 3) & ~3; // round up to next word
		
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
		
        int h = hashPtr(k);
		while (true)
		{
			int oldHead = m_client.r(h);
			m_client.w(newPtr, oldHead);
			if (m_client.cas(h, oldHead, newPtr) != 0) break;
		}
	}

	public boolean has(byte[] k) {
        return findKey(k) != 0;
	}

    public boolean has(String key) {
        return has(key.getBytes());
    }

    public String getKey(int ptr)
    {
        int len = m_client.r(ptr + 8);
        byte[] bytes = m_client.readbytes(ptr + 12, len);
        return new String(bytes);
    }

    public int getNBins()
    {
        return N;
    }

    public int getHead(int bin)
    {
        return m_client.r(4*(N+bin));
    }

    public int getNext(int ptr)
    {
        return m_client.r(ptr);
    }
}
