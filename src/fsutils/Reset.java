package fsutils;

import common.ActiveRDMA;
import client.*;
import java.io.*;

public class Reset
{
    public static void main(String[] args)
    {
        if (args.length < 1)
        {
            System.err.println("Usage: Reset <hostname>");
            return;
        }

        Client c;
        try
        {
            c = new Client(args[0]);
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return;
        }

        c.w(0, 4); // reset alloc
        for (int i = 0; i < 16384; i += 4) // blow away the hashtable
            c.w(i, 0);
    }
}
