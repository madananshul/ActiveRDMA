package examples.dht;

import common.ActiveRDMA;

public class DHT_Active implements examples.dht.DHT {

	static final int N = 1024;

	protected ActiveRDMA m_c;

	public static class DHT_Active_Get {

		static int hash(int[] key) {
			int t = 0;
			for (int i = 0; i < key.length; i++) t ^= key[i];
			return t % N;
		}

		static boolean compareKey(ActiveRDMA c, int ptr, int[] args) {
			for (int i = 0; i < args.length; i++)
				if (c.r(ptr + 2 + i) != args[i]) return false;

			return true;
		}

		public static int execute(ActiveRDMA c, int[] args) {

			// args[] is the key

			int h = hash(args);
			int ptr = c.r(N + h);
			while (ptr != 0)
			{
				if (compareKey(c, ptr, args)) break;
				ptr = c.r(ptr);
			}

			if (ptr != 0)
				return c.r(ptr + 1);
			else
				return 0;
		}
	}

	public static class DHT_Active_Has {
		static final int N = 1024;

		public static class DHT_Active_Get {

			static int hash(int[] key) {
				int t = 0;
				for (int i = 0; i < key.length; i++) t ^= key[i];
				return t % N;
			}

			static boolean compareKey(ActiveRDMA c, int ptr, int[] args) {
				for (int i = 0; i < args.length; i++)
					if (c.r( ptr + 2 + i ) != args[i]) return false;

				return true;
			}

			public static int execute(ActiveRDMA c, int[] args) {

				// args[] is the key

				int h = hash(args);
				int ptr = c.r( N + h );
				while (ptr != 0)
				{
					if (compareKey(c, ptr, args)) break;
					ptr = c.r( ptr );
				}

				return ptr != 0 ? 1 : 0;
			}
		}
	}

	public static class DHT_Active_Put {

		static int hash(int[] key, int off) {
			int t = 0;
			for (int i = off; i < key.length; i++) t ^= key[i];
			return t % N;
		}

		public static int execute(ActiveRDMA c, int[] args) {

			// args[0] is the value, args[1...] is the key
			int val = args[0];
			int kLen = args.length - 1;

			// allocate new block: <next> <val> <key>...
			int newPtr = 0;
			while (true)
			{
				newPtr = c.r(0);
				if (c.cas(0, newPtr, newPtr + 2 + kLen) !=0)
					break;
			}

			int h = hash(args, 1);
			c.w(newPtr+1, val);
			for (int i = 0; i < kLen; i++)
				c.w(newPtr + 2 + i, args[i+1]);

			while (true)
			{
				int head = c.r( N + h );
				c.w(newPtr, head);
				if (c.cas(N + h, head, newPtr) != 0 )
					break;
			}

			return newPtr;
		}
	}

	public DHT_Active(ActiveRDMA c) {
		m_c = c;

		c.load(DHT_Active_Get.class);
		c.load(DHT_Active_Put.class);
		c.load(DHT_Active_Has.class);
		c.w(0, 2*N);

		for (int i = 0; i < N; i++)
			c.w(N + i, 0);
	}

	int[] stringToInt(String s)
	{
		byte[] bytes = s.getBytes();
		int[] ret = new int[(bytes.length+3)/4];
		for (int i = 0; i < ret.length; i++) ret[i] = 0;

		for (int i = 0; i < bytes.length; i++)
		{
			ret[i/4] <<= 8;
			ret[i/4] |= bytes[i];
		}

		return ret;
	}

	public int get(String key) {
		System.out.println("Input name to get is " + key);
		int[] k = stringToInt(key);

		return m_c.run(DHT_Active_Get.class, k);
	}

	public boolean has(String key) {
		int[] k = stringToInt(key);

		return m_c.run(DHT_Active_Has.class, k) != 0;
	}

	public void put(String key, int val) {
		int[] k = stringToInt(key);
		int[] args = new int[k.length + 1];
		System.out.println("Input inode to put is " + val);
		args[0] = val;
		for (int i = 0; i < k.length; i++)
			args[i + 1] = k[i];

		m_c.run(DHT_Active_Put.class, args);
	}
}
