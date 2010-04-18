package dfs;

import common.*;
import examples.dht.*;

/*
  Inodes and single/double/triple indirect blocks are allocated in 1K-word
  (4K-byte) blocks/pages. Leaf pointers point to 4K data pages. Data is
  stored in little-endian order.

  Inode structure is:

   [0] len, in words
   [1...1022] direct ptrs
   [1023] doubly-indirect ptr ( -> level 2 -> level 3 -> data)
*/

public class DFS_RDMA implements DFS
{
    ActiveRDMA m_client;
    DHT m_dht;

    public DFS_RDMA(ActiveRDMA client)
    {
        m_client = client;
        m_dht = new DHT_RDMA(client, 1024);
    }

    private int alloc(int len)
    {
        int ptr = 0;
        while (true)
        {
            ptr = m_client.r(0);
            if (m_client.cas(0, ptr, ptr + len) != 0) break;
        }

        for (int i = 0; i < len; i++)
            m_client.w(ptr + i, 0);

        return ptr;
    }

    private int getBlock(int inode, int blockno)
    {
        if (blockno <= 1022)
            return m_client.r(inode + 1 + blockno);
        else
        {
            blockno -= 1022;
            int idx0 = blockno / 1024;
            int idx1 = blockno % 1024;

            int l2 = m_client.r(inode + 1023);
            if (l2 == 0) return 0;
            int l3 = m_client.r(l2 + idx0);
            if (l3 == 0) return 0;
            return m_client.r(l3 + idx1);
        }
    }

    private void setBlock(int inode, int blockno, int ptr)
    {
        if (blockno <= 1022)
            m_client.w(inode + 1 + blockno, ptr);
        else
        {
            blockno -= 1022;
            int idx0 = blockno / 1024;
            int idx1 = blockno % 1024;

            int l2 = m_client.r(inode + 1023);
            if (l2 == 0)
            {
                l2 = alloc(1024);
                m_client.w(inode + 1023, l2);
            }
            int l3 = m_client.r(l2 + idx0);
            if (l3 == 0)
            {
                l3 = alloc(1024);
                m_client.w(l2 + idx0, l3);
            }
            m_client.w(l3 + idx1, ptr);
        }
    }

    public int lookup(String name)
    {
        return m_dht.get(name);
    }

    public int create(String name)
    {
        int inode = alloc(1024);
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

    private int access(int inode, int[] buffer, int off, int len, boolean write)
    {
        int fLen = getLen(inode);
        System.out.println("off " + off + " len " + len + " fLen " + fLen);
        System.out.println("buf len " + buffer.length);
        
        //Why is this condition here, seems to be not valid for writes??
        //if (off + len > fLen) return -1;
        
        if (len > buffer.length) return -1;
        System.out.println("Access(Read/Write) is going to happen.");

        //Are we updating length when writing to inode using put??
        //dfs.getLen() seems to be returning 0 even after a write??
        
        int bufptr = 0;
        while (len > 0)
        {
            int block = off / 1024;
            int ptr = getBlock(inode, block);
            if (ptr == 0)
            {
                if (!write) return -1;
                ptr = alloc(1024);
                setBlock(inode, block, ptr);
            }

            for (int i = off % 1024; i < (block+1)*1024 && len > 0; i++, len--)
                if (write)
                    m_client.w(ptr + i, buffer[bufptr++]);
                else
                    buffer[bufptr++] = m_client.r(ptr + i);
        }

        return 0;
    }

    public int get(int inode, int[] buffer, int off, int len)
    {
        return access(inode, buffer, off, len, false);
    }

    public int put(int inode, int[] buffer, int off, int len)
    {
        return access(inode, buffer, off, len, true);
    }
}