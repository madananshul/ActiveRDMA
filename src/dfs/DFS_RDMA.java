package dfs;

import common.*;
import examples.dht.*;

/*
   This is a generic inode-level store that uses the DHT to implement
   name->inode mapping. Arbitrary higher-level structure can be built on this.

   Note that the interface does *NOT* work like a traditional filesystem API --
   specifically, length and content are managed separately. You must setLen()
   to lengthen a file before writing to a range past the end.

   Inodes and single/double/triple indirect blocks are allocated in 1K-word
   (4K-byte) blocks/pages. Leaf pointers point to 4K data pages.

   Inode structure is:

    [0] len, in words
    [1...1022] direct ptrs
    [1023] doubly-indirect ptr ( -> level 2 -> level 3 -> data)
*/

public class DFS_RDMA implements DFS
{
    protected ActiveRDMA m_client;
    private DHT m_dht;

    public DFS_RDMA(ActiveRDMA client)
    {
        m_client = client;
        m_dht = new DHT_RDMA(client, 1024);
    }

    public DFS_RDMA(ActiveRDMA client, boolean noInit)
    {
        m_client = client;
        m_dht = new DHT_RDMA(client, 1024, noInit);
    }

    public int alloc(int len)
    {
        int ptr = 0;
        while (true)
        {
            ptr = m_client.r(0);
            if (m_client.cas(0, ptr, ptr + len) != 0) break;
        }

        return ptr;
    }

    public int getBlock(int inode, int blockno)
    {
        if (blockno <= 1022)
            return m_client.r(inode + 4 * (1 + blockno));
        else
        {
            blockno -= 1022;
            int idx0 = blockno / 1024;
            int idx1 = blockno % 1024;

            int l2 = m_client.r(inode + 4 * 1023);
            if (l2 == 0) return 0;
            int l3 = m_client.r(l2 + 4*idx0);
            if (l3 == 0) return 0;
            return m_client.r(l3 + 4*idx1);
        }
    }

    public void setBlock(int inode, int blockno, int ptr)
    {
        if (blockno <= 1022)
            m_client.w(inode + 4 * (1 + blockno), ptr);
        else
        {
            blockno -= 1022;
            int idx0 = blockno / 1024;
            int idx1 = blockno % 1024;

            int l2 = m_client.r(inode + 4 * 1023);
            if (l2 == 0)
            {
                l2 = alloc(4 * 1024);
                m_client.w(inode + 4 * 1023, l2);
            }
            int l3 = m_client.r(l2 + 4 * idx0);
            if (l3 == 0)
            {
                l3 = alloc(4 * 1024);
                m_client.w(l2 + 4 * idx0, l3);
            }
            m_client.w(l3 + 4 * idx1, ptr);
        }
    }

    public int lookup(String name)
    {
        return m_dht.get(name);
    }

    public int create(String name)
    {
        int inode = alloc(4 * 1024);
        m_dht.put(name, inode);
        return inode;
    }

    public void setLen(int inode, int len)
    {
        m_client.w(inode, len);
    }

    public int getLen(int inode)
    {
        return m_client.r(inode);
    }
    
    public int[] find(int inode, String pattern)
    {
        return m_dht.match(inode, pattern.getBytes());
    }
    
    public int[] find(int inode, byte[] pattern)
    {
        return m_dht.match(inode, pattern);
    }

    protected int access(int inode, byte[] buffer, int off, int len, boolean write)
    {
        int fLen = getLen(inode);
        
        if (off + len > fLen) return -1;
        
        if (len > buffer.length) return -1;

        int bufptr = 0;

        byte[] buf = new byte[1024];

        while (len > 0)
        {
            int block = off / 4096;
            int blockLen = len < 1024 ? len : 1024; // keep each req < 1K (for Ethernet frame size)
            int blockOff = off % 4096;
            if (blockOff + blockLen > 4096) blockLen = 4096 - blockOff;

            //System.out.println(String.format("block %d blockLen %d blockOff %d len %d", block, blockLen, blockOff, len));

            int ptr = getBlock(inode, block);
            if (ptr == 0)
            {
                if (!write) return -1;
                ptr = alloc(4 * 1024);
                setBlock(inode, block, ptr);
            }

            if (write)
            {
                if (blockLen == 1024)
                {
                    System.arraycopy(buffer, bufptr, buf, 0, blockLen);
                    m_client.writebytes(ptr + blockOff, buf);
                }
                else
                {
                    byte[] smallbuf = new byte[blockLen];
                    System.arraycopy(buffer, bufptr, smallbuf, 0, blockLen);
                    m_client.writebytes(ptr + blockOff, smallbuf);
                }
            }
            else
            {
                byte[] result = m_client.readbytes(ptr + blockOff, blockLen);
                System.arraycopy(result, 0, buffer, bufptr, blockLen);
            }

            bufptr += blockLen;
            len -= blockLen;
            off += blockLen;
        }

        return 0;
    }

    public int get(int inode, byte[] buffer, int off, int len)
    {
        return access(inode, buffer, off, len, false);
    }

    public int put(int inode, byte[] buffer, int off, int len)
    {
        return access(inode, buffer, off, len, true);
    }
}
