package examples.dht;

import common.ActiveRDMA;

public class DHT_Active implements examples.dht.DHT {

	static final int N = 1024;

	protected ActiveRDMA m_c;

    protected DHT_RDMA m_dht;

	public static class DHT_Active_Get {

		public static int execute(ActiveRDMA c, int[] args) {

			// args[] is the key
            byte[] k = new byte[args.length];
            for (int i = 0; i < args.length; i++)
                k[i] = (byte)args[i];

            DHT_RDMA r = new DHT_RDMA(c, N, true);
            return r.get(k);
        }
	}
	
	public static class DHT_Active_Find {

		public static int[] execute(ActiveRDMA c, int[] args) {

			// args[1] is the key
            byte[] pattern = new byte[args.length-1];
            for (int i = 0; i < args.length; i++)
                pattern[i] = (byte)args[i];

            DHT_RDMA r = new DHT_RDMA(c, N, true);
            return r.match(args[0], pattern);
        }
	}

	public static class DHT_Active_Has {

		public static int execute(ActiveRDMA c, int[] args) {

			// args[] is the key
            byte[] k = new byte[args.length];
            for (int i = 0; i < args.length; i++)
                k[i] = (byte)args[i];

            DHT_RDMA r = new DHT_RDMA(c, N, true);
            return r.has(k) ? 1 : 0;
        }
	}

	public static class DHT_Active_Put {

		public static int execute(ActiveRDMA c, int[] args) {
			// args[] is the value, followed by the key
            byte[] k = new byte[args.length - 1];
            for (int i = 0; i < args.length - 1; i++)
                k[i] = (byte)args[i + 1];

            DHT_RDMA r = new DHT_RDMA(c, N, true);
            r.put(k, args[0]);
            return 0;
		}
	}

	public DHT_Active(ActiveRDMA c) {
		m_c = c;
        m_dht = new DHT_RDMA(c, N);

        c.load(DHT_RDMA.class);
        c.load(DHT_Active_Find.class);
		c.load(DHT_Active_Get.class);
		c.load(DHT_Active_Put.class);
		c.load(DHT_Active_Has.class);
	}

	
	public int[] match(int key, byte[] pattern) {
		//byte[] b = pattern.getBytes();

		int[] args = new int[pattern.length+1];
		args[0] = key;
        for (int i = 0; i < pattern.length; i++)
            args[i] = (int)pattern[i];

		return m_c.runArray(DHT_Active_Find.class, args);
	}
	
	public int get(String key) {
        byte[] b = key.getBytes();

		int[] args = new int[b.length];
        for (int i = 0; i < b.length; i++)
            args[i] = (int)b[i];

		return m_c.run(DHT_Active_Get.class, args);
	}

	public boolean has(String key) {
        byte[] b = key.getBytes();

		int[] args = new int[b.length];
        for (int i = 0; i < b.length; i++)
            args[i] = (int)b[i];

		return m_c.run(DHT_Active_Has.class, args) != 0;
	}

	public void put(String key, int val) {
        byte[] b = key.getBytes();

		int[] args = new int[b.length + 1];
        for (int i = 0; i < b.length; i++)
            args[i + 1] = (int)b[i];

        args[0] = val;

		m_c.run(DHT_Active_Put.class, args);
	}
}
