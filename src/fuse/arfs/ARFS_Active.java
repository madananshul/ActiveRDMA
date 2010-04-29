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

public class ARFS_Active extends ARFS
{
    static final int GETATTR = 1;
    static final int LINK = 2;
    static final int MKDIR = 3;
    static final int MKNOD = 4;
    static final int UNLINK = 5;
    static final int TRUNCATE = 6;

    public static class ARFS_Active_Adapter
    {
        public static int[] execute(ActiveRDMA client, int[] args)
        {
            ARFS arfs = new ARFS(client, true);
            int[] result = null;

            switch (args[0])
            {
                case GETATTR:
                    result = arfs.getattr(ActiveRDMA.getString(args, 1));
                    break;
                case LINK:
                    String[] strings = ActiveRDMA.getStrings(args, 1);
                    result = new int[] { arfs.link(strings[0], strings[1]) };
                    break;
                case MKDIR:
                    result = new int[] { arfs.mkdir(ActiveRDMA.getString(args, 1)) };
                    break;
                case MKNOD:
                    result = new int[] { arfs.mknod(ActiveRDMA.getString(args, 1)) };
                    break;
                case UNLINK:
                    result = new int[] { arfs.unlink(ActiveRDMA.getString(args, 1)) };
                    break;
                case TRUNCATE:
                    result = new int[] { arfs.truncate(ActiveRDMA.getString(args, 2), args[1]) };
                    break;
            }

            return result;
        }
    }

    public ARFS_Active(ActiveRDMA client, boolean noInit)
    {
        super(client, noInit, new DFS_Active(client, noInit));

        client.load(ARFS.class);
        client.load(ARFS_Active_Adapter.class);
    }

    public int[] getattr(String path)
    {
        int[] args = ActiveRDMA.constructArgs(1, path);
        args[0] = GETATTR;

        return m_client.runArray(ARFS_Active_Adapter.class, args);
    }

    public int link(String from, String to)
    {
        int[] args = ActiveRDMA.constructArgs(1, from, to);
        args[0] = LINK;

        return m_client.runArray(ARFS_Active_Adapter.class, args) [0];
    }

    public int mkdir(String path)
    {
        int[] args = ActiveRDMA.constructArgs(1, path);
        args[0] = MKDIR;

        return m_client.runArray(ARFS_Active_Adapter.class, args) [0];
    }

    public int mknod(String path)
    {
        int[] args = ActiveRDMA.constructArgs(1, path);
        args[0] = MKNOD;

        return m_client.runArray(ARFS_Active_Adapter.class, args) [0];
    }

    public int unlink(String path)
    {
        int[] args = ActiveRDMA.constructArgs(1, path);
        args[0] = UNLINK;

        return m_client.runArray(ARFS_Active_Adapter.class, args) [0];
    }

    public int truncate(String path, int len)
    {
        int[] args = ActiveRDMA.constructArgs(2, path);
        args[0] = TRUNCATE;
        args[1] = len;

        return m_client.runArray(ARFS_Active_Adapter.class, args) [0];
    }
}
