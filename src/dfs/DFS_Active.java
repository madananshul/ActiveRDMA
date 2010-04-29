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

    public DFS_Active(ActiveRDMA c)
    {
        super(c);

        c.load(DFS_RDMA.class);
        c.load(DFS_Active_Adapter.class);
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
}
