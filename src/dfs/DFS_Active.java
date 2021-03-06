package dfs;

import common.*;

public class DFS_Active extends DFS_RDMA
{
    public static class DFS_Active_Adapter
    {
        static DFS_RDMA rdma;

        static String getS(int[] args, int off)
        {
            byte[] bytes = new byte[args.length - off];
            for (int i = off; i < args.length; i++)
                bytes[i - off] = (byte)args[i];

            return new String(bytes);
        }

        public static int execute(ActiveRDMA c, int[] args)
        {
            if (rdma == null)
                rdma = new DFS_RDMA(c, true);

            switch (args[0])
            {
                case 1: // create
                    return rdma.create(getS(args, 1));
                case 2: // lookup
                    return rdma.lookup(getS(args, 1));
                case 3: // setLen
                    rdma.setLen(args[1], args[2]);
                    return 0;
                case 4: // getLen
                    return rdma.getLen(args[1]);
                case 5: // getBlock
                    return rdma.getBlock(args[1], args[2]);
                case 6: // setBlock
                    rdma.setBlock(args[1], args[2], args[3]);
                    return 0;
                case 7: // alloc
                    return rdma.alloc(args[1]);
                default:
                    return -1;
            }
        }
    }

    public static class DFS_Active_IO_Adapter
    {
        static DFS_RDMA rdma;

        public static int MAX_BYTES_IN_REQ = 1200; // 1200-byte packets, to leave some headroom for 1500-byte Eth packets

        public static int[] execute(ActiveRDMA c, int[] args)
        {
            // arg 0 is {0: get, 1: put}, 1 is inode, 2 is off, 3 is len,
            // following is data for put
            // returns nothing for put, data for get
            int op = args[0];
            int inode = args[1];
            int off = args[2];
            int len = args[3];

            if (rdma == null)
                rdma = new DFS_RDMA(c, true);

            if (op == 0)
            {
                byte[] buf = new byte[len];
                rdma.get(inode, buf, off, len);
                return ActiveRDMA.pack(0, buf, 0, buf.length);
            }
            else if (op == 1)
            {
                byte[] buf = ActiveRDMA.unpack(args, 4);
                rdma.put(inode, buf, off, len);
                return new int[] { };
            }
            else if (op ==2){
            	byte[] buf = ActiveRDMA.unpack(args, 1);
                return rdma.find(inode, buf);
            }

            return new int[] { };
        }
    }

    public DFS_Active(ActiveRDMA c)
    {
        super(c);

        c.load(DFS_RDMA.class);
        c.load(DFS_Active_Adapter.class);
        c.load(DFS_Active_IO_Adapter.class);
    }

    public DFS_Active(ActiveRDMA c, boolean noInit)
    {
        super(c, noInit);

        c.load(DFS_RDMA.class);
        c.load(DFS_Active_Adapter.class);
        c.load(DFS_Active_IO_Adapter.class);
    }

    int[] constructArgs(int prefix, String s)
    {
        byte[] sBytes = (s != null) ? s.getBytes() : null;
        int[] args = new int[prefix + ((sBytes != null) ? sBytes.length : 0)];
        if (sBytes != null)
            for (int i = 0; i < sBytes.length; i++)
                args[prefix + i] = (int)sBytes[i];

        return args;
    }

    // these override the methods in the RDMA class to channel the computation onto the server
    public int create(String s)
    {
        int[] args = constructArgs(1, s);
        args[0] = 1;

        return m_client.run(DFS_Active_Adapter.class, args);
    }

    public int lookup(String s)
    {
        int[] args = constructArgs(1, s);
        args[0] = 2;

        return m_client.run(DFS_Active_Adapter.class, args);
    }
    

    public void setLen(int inode, int len)
    {
        int[] args = new int[] { 3, inode, len };
        m_client.run(DFS_Active_Adapter.class, args);
    }

    public int getLen(int inode)
    {
        int[] args = new int[] { 4, inode };
        return m_client.run(DFS_Active_Adapter.class, args);
    }

    public int getBlock(int inode, int block)
    {
        int[] args = new int[] { 5, inode, block };
        return m_client.run(DFS_Active_Adapter.class, args);
    }

    public void setBlock(int inode, int block, int ptr)
    {
        int[] args = new int[] { 6, inode, block, ptr };
        m_client.run(DFS_Active_Adapter.class, args);
    }

    public int alloc(int size)
    {
        int[] args = new int[] { 7, size };
        return m_client.run(DFS_Active_Adapter.class, args);
    }
    
    public int[] find(int inode, String pattern)
    {
        int[] args = constructArgs(1, pattern);
        args[0] = 8;

        return m_client.runArray(DFS_Active_IO_Adapter.class, args);
    }

    public int get(int inode, byte[] buffer, int off, int len)
    {
        int buf_off = 0;
        while (len > 0)
        {
            int chunk = len;
            if (chunk > DFS_Active_IO_Adapter.MAX_BYTES_IN_REQ)
                chunk = DFS_Active_IO_Adapter.MAX_BYTES_IN_REQ;

            int[] args = new int[4];
            args[0] = 0;
            args[1] = inode;
            args[2] = off;
            args[3] = chunk;

            int[] dat = m_client.runArray(DFS_Active_IO_Adapter.class, args);
            if (dat == null)
                return -1;

            ActiveRDMA.unpack(dat, 0, buffer, buf_off);

            off += chunk;
            buf_off += chunk;
            len -= chunk;
        }
        
        return 0;
    }

    public int put(int inode, byte[] buffer, int off, int len)
    {
        int buf_off = 0;

        while (len > 0)
        {
            int chunk = len;
            if (chunk > DFS_Active_IO_Adapter.MAX_BYTES_IN_REQ)
                chunk = DFS_Active_IO_Adapter.MAX_BYTES_IN_REQ;

            int[] args = ActiveRDMA.pack(4, buffer, buf_off, chunk);
            args[0] = 1;
            args[1] = inode;
            args[2] = off;
            args[3] = chunk;

            int[] ret = m_client.runArray(DFS_Active_IO_Adapter.class, args);

            off += chunk;
            buf_off += chunk;
            len -= chunk;
        }

        return 0;
    }
}
