package fsutils;

import common.ActiveRDMA;
import client.*;
import dfs.*;
import java.io.*;

public class Find
{
    public static class Active_Finder
    {
        static final int MAX_RET = 300; // 1200 bytes

        public static int[] execute(ActiveRDMA c, int[] args)
        {
            DFS dfs = new DFS_RDMA(c, true);

            int lastBin = args[0];
            int lastPtr = args[1];
            String pattern = ActiveRDMA.getString(args, 2);

            int[] ret = new int[2];   

            DFSIter iter = new DFSIter(dfs, lastBin, lastPtr);
            while (!iter.done())
            {
                iter.findNext(pattern);
                if (iter.done()) break;

                int[] fname = ActiveRDMA.pack(0, iter.key().getBytes());
                if (ret.length + fname.length > MAX_RET) break;

                ret = ActiveRDMA.appendArray(ret, fname);
                iter.next();
            }

            ret[0] = iter.bin();
            ret[1] = iter.ptr();

            return ret;
        }
    }

    static String DIR_PREFIX = "###___DIR___###";

    static void printResults(int[] results)
    {
        int off = 2;
        while (off < results.length)
        {
            String val = new String(ActiveRDMA.unpack(results, off));
            off += (results[off]+3)/4 + 1;

            boolean dir = false;
            if (val.startsWith(DIR_PREFIX))
            {
                dir = true;
                val = val.substring(DIR_PREFIX.length());
            }

            if (dir)
                System.out.println("dir: " + val);
            else
                System.out.println("file: " + val);
        }
    }

    public static void main(String[] args)
    {
        if (args.length < 3)
        {
            System.err.println("Usage: Find <hostname> <active | rdma> <pattern>");
            return;
        }

        Client c;
        try {
            c = new Client(args[0]);
        } catch (IOException e)
        {
            System.err.println(e.toString());
            return;
        }

        boolean active = args[1].equals("active");
        String pattern = args[2];

        int[] arg = ActiveRDMA.constructArgs(2, pattern);
        arg[0] = 0;
        arg[1] = 0;
        
        if (active)
        {
            c.load(Active_Finder.class);
            while (true)
            {
                int[] results = c.runArray(Active_Finder.class, arg);
                if (results.length == 2) break;

                printResults(results);
                arg[0] = results[0];
                arg[1] = results[1];
            }
        }
        else
        {
            while (true)
            {
                int[] results = Active_Finder.execute(c, arg);
                if (results.length == 2) break;

                printResults(results);
        
                arg[0] = results[0];
                arg[1] = results[1];
            }
        }
    }
}
