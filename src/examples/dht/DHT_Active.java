package examples.dht;

import java.util.concurrent.atomic.AtomicInteger;
import common.*;

public class DHT_Active implements examples.dht.DHT {

    static final int N = 1024;

    protected ExtActiveRDMA m_c;

    public static class DHT_Active_Get {

	static int hash(int[] key) {
            int t = 0;
            for (int i = 0; i < key.length; i++) t ^= key[i];
            return t % N;
	}

        static boolean compareKey(AtomicInteger[] mem, int ptr, int[] args) {
            for (int i = 0; i < args.length; i++)
                if (mem[ptr + 2 + i].get() != args[i]) return false;

            return true;
        }

        public static int execute(AtomicInteger[] mem, int[] args) {
            
            // args[] is the key
            
            int h = hash(args);
            int ptr = mem[N + h].get();
            while (ptr != 0)
            {
                if (compareKey(mem, ptr, args)) break;
                ptr = mem[ptr].get();
            }

            if (ptr != 0)
                return mem[ptr + 1].get();
            else
                return 0;
        }
    }

    public static class DHT_Active_Has {
        static final int N = 1024;

        protected ExtActiveRDMA m_c;

        public static class DHT_Active_Get {

            static int hash(int[] key) {
                int t = 0;
                for (int i = 0; i < key.length; i++) t ^= key[i];
                return t % N;
            }

            static boolean compareKey(AtomicInteger[] mem, int ptr, int[] args) {
                for (int i = 0; i < args.length; i++)
                    if (mem[ptr + 2 + i].get() != args[i]) return false;

                return true;
            }

            public static int execute(AtomicInteger[] mem, int[] args) {
            
                // args[] is the key
            
                int h = hash(args);
                int ptr = mem[N + h].get();
                while (ptr != 0)
                {
                    if (compareKey(mem, ptr, args)) break;
                    ptr = mem[ptr].get();
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

        public static int execute(AtomicInteger[] mem, int[] args) {
            
            // args[0] is the value, args[1...] is the key
            int val = args[0];
            int kLen = args.length - 1;

            // allocate new block: <next> <val> <key>...
            int newPtr = 0;
            while (true)
            {
                newPtr = mem[0].get();
                if (mem[0].compareAndSet(newPtr, newPtr + 2 + kLen))
                    break;
            }

            int h = hash(args, 1);
            mem[newPtr + 1].set(val);
            for (int i = 0; i < kLen; i++)
                mem[newPtr + 2 + i].set(args[i + 1]);

            while (true)
            {
                int head = mem[N + h].get();
                mem[newPtr].set(head);
                if (mem[N + h].compareAndSet(head, newPtr))
                    break;
            }

            return newPtr;
        }
    }

    public DHT_Active(ExtActiveRDMA c) {
        m_c = c;

        c.load(DHT_Active_Get.class);
        c.load(DHT_Active_Put.class);
        c.w(0, 0);

        for (int i = 0; i < N; i++)
            c.w(N + i, 0);
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

    public int get(String key) {
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

        args[0] = val;
        for (int i = 0; i < k.length; i++)
            args[i + 1] = k[i];

        m_c.run(DHT_Active_Put.class, args);
    }
}
