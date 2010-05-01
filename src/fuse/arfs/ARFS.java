package fuse.arfs;

import junit.framework.TestCase;
import client.Client;
import common.ActiveRDMA;
import dfs.*;

import fuse.*;
//import fuse.compat.Filesystem2;
import fuse.Filesystem3;
import fuse.compat.FuseDirEnt;
import fuse.compat.FuseStat;

import java.io.*;
import java.nio.*;
import java.util.Collection;
import java.util.Enumeration;
import java.util.Iterator;

import org.apache.commons.logging.LogFactory;
import org.apache.commons.logging.Log;

import client.Client;

import common.ActiveRDMA;

public class ARFS
{
    final String server = "localhost";

    public DFS dfs;

    final String dir_prefix = "###___DIR___###";

    protected ActiveRDMA m_client;

    public ARFS(ActiveRDMA client, boolean noInit)
    {
        m_client = client;

        if (noInit)
        {
            dfs = new DFS_RDMA(client, noInit);
        }
        else
            dfs = new DFS_RDMA(client);

        if (!noInit)
            dfs.create(dir_prefix + "/");
    }

    protected ARFS(ActiveRDMA client, boolean noInit, DFS _dfs)
    {
        m_client = client;
        dfs = _dfs;

        if (!noInit)
            dfs.create(dir_prefix + "/");
    }

    int appendDirEntry(int dirInode, String name, int inode)
    {
        byte[] dirEntry = null;
        try
        {
            ByteArrayOutputStream oub = new ByteArrayOutputStream();
            DataOutputStream out = new DataOutputStream(oub);

            byte[] nameB = name.getBytes();
            if (nameB.length > 255) return -1;

            out.writeInt(inode);
            out.writeByte(nameB.length);
            out.write(nameB, 0, nameB.length);

            dirEntry = oub.toByteArray();
        } catch (IOException e) {
        }

        int len = dfs.getLen(dirInode);
        dfs.setLen(dirInode, len + dirEntry.length);
        dfs.put(dirInode, dirEntry, len, dirEntry.length);

        return 0;
    }

    public static String dirname(String path)
    {
        int idx = path.lastIndexOf('/');
        if (idx == -1 || idx == 0)
            return new String("/");
        else
            return path.substring(0, idx);
    }

    public static String basename(String path)
    {
        int idx = path.lastIndexOf('/');
        if (idx == -1)
            return path;
        else
            return path.substring(idx + 1);
    }

    // returns: { inode (-1 for no ent), len, isDir }
    public int[] getattr(String path)
    {
        boolean dir = false;
        int inode = dfs.lookup(path);
        int len = -1;
        if (inode == 0 || (len = dfs.getLen(inode)) == -1)
        {
            inode = dfs.lookup(dir_prefix + path);
            dir = true;
        }
        if (inode == 0 || (len = dfs.getLen(inode)) == -1)
            inode = -1;

        return new int[] { inode, len, dir ? 1 : 0 };
    }

    public int link(String from, String to)
    {
        int inode = dfs.lookup(from);
        if (inode == 0)
            return -1;

        int toInode = dfs.lookup(to);
        if (inode != 0)
            return -1;

        int dirInode = dfs.lookup(dir_prefix + dirname(to));
        if (dirInode == 0)
            return -1;

        return appendDirEntry(dirInode, basename(to), inode);
    }

    public int mkdir(String path)
    {
        int fileInode = dfs.lookup(path);
        if (fileInode != 0 && dfs.getLen(fileInode) != -1)
            return -1;

        int prevInode = dfs.lookup(dir_prefix + path);
        if (prevInode != 0)
        {
            if (dfs.getLen(prevInode) != -1)
                return -1;

            dfs.setLen(prevInode, 0);
            return 0;
        }

        int parent_inode = dfs.lookup(dir_prefix + dirname(path));
        if (parent_inode == 0)
            return -1;

        int inode = dfs.create(dir_prefix + path);
        return appendDirEntry(parent_inode, basename(path), inode);
    }

    public int mknod(String path)
    {
        int dirInode = dfs.lookup(dir_prefix + path);
        if (dirInode != 0 && dfs.getLen(dirInode) != -1)
            return -1;

        int prevInode = dfs.lookup(path);
        if (prevInode != 0)
        {
            if (dfs.getLen(prevInode) != -1)
                return -1;

            // already existed: just set length to 0 and return
            dfs.setLen(prevInode, 0);
            return 0;
        }

        int parent_inode = dfs.lookup(dir_prefix + ARFS.dirname(path));
        if (parent_inode == 0)
            return -1;

        int inode = dfs.create(path);
        appendDirEntry(parent_inode, ARFS.basename(path), inode);
        return 0;
    }

    public int unlink(String path)
    {
        int fInode = dfs.lookup(path);
        if (fInode == 0 || dfs.getLen(fInode) == -1)
            fInode = dfs.lookup(dir_prefix + path);
        if (fInode == 0 || dfs.getLen(fInode) == -1)
            return -1;

        dfs.setLen(fInode, -1);

        return 0;
    }

    public int truncate(String path, int size)
    {
        int inode = dfs.lookup(path);
        if (inode == 0)
            return -1;

        dfs.setLen(inode, (int)size);

        return 0;
    }
}


