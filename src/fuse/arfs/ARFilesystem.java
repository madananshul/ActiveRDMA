/**
 *   FUSE-J: Java bindings for FUSE (Filesystem in Userspace by Miklos Szeredi (mszeredi@inf.bme.hu))
 *
 *   Copyright (C) 2003 Peter Levart (peter@select-tech.si)
 *
 *   This program can be distributed under the terms of the GNU LGPL.
 *   See the file COPYING.LIB
 */

package fuse.arfs;

import junit.framework.TestCase;
import client.Client;
import common.ActiveRDMA;
import dfs.*;

import fuse.*;
import fuse.compat.Filesystem1;
import fuse.compat.FuseDirEnt;
import fuse.compat.FuseStat;
import fuse.arfs.util.Node;
import fuse.arfs.util.Tree;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.util.Collection;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.apache.commons.logging.LogFactory;
import org.apache.commons.logging.Log;

import client.Client;

import common.ActiveRDMA;


public class ARFilesystem implements Filesystem1
{
   private static final Log log = LogFactory.getLog(ARFilesystem.class);

   private static final int blockSize = 512;
   
   final String server = "10.0.0.2";

   private ZipFile zipFile;
   private long zipFileTime;
   private ZipEntry rootEntry;
   private Tree tree;
   private FuseStatfs statfs;

   private ZipFileDataReader zipFileDataReader;


   public ARFilesystem() throws IOException
   {
      rootEntry = new ZipEntry("")
      {
         public boolean isDirectory()
         {
            return true;
         }
      };
      //rootEntry.setTime(zipFileTime);
      rootEntry.setSize(0);

      tree = new Tree();
      tree.addNode(rootEntry.getName(), rootEntry);

      statfs = new FuseStatfs();
      statfs.blocks = 0;
      statfs.blockSize = blockSize;
      statfs.blocksFree = 0;
      statfs.files = 0;
      statfs.filesFree = 0;
      statfs.namelen = 2048;

   }


   public void chmod(String path, int mode) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void chown(String path, int uid, int gid) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public FuseStat getattr(String path) throws FuseException
   {
      Node node = tree.lookupNode(path);
      ZipEntry entry = null;
      if (node == null || (entry = (ZipEntry)node.getValue()) == null)
         throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

      FuseStat stat = new FuseStat();

      stat.mode = entry.isDirectory() ? FuseFtype.TYPE_DIR | 0755 : FuseFtype.TYPE_FILE | 0644;
      stat.nlink = 1;
      stat.uid = 0;
      stat.gid = 0;
      stat.size = entry.getSize();
      stat.atime = stat.mtime = stat.ctime = (int) (entry.getTime() / 1000L);
      stat.blocks = (int) ((stat.size + 511L) / 512L);

      return stat;
   }

   public FuseDirEnt[] getdir(String path) throws FuseException
   {
      Node node = tree.lookupNode(path);
      ZipEntry entry = null;
      if (node == null || (entry = (ZipEntry)node.getValue()) == null)
         throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

      if (!entry.isDirectory())
         throw new FuseException("Not A Directory").initErrno(FuseException.ENOTDIR);

      Collection children = node.getChildren();
      FuseDirEnt[] dirEntries = new FuseDirEnt[children.size()];

      int i = 0;
      for (Iterator iter = children.iterator(); iter.hasNext(); i++)
      {
         Node childNode = (Node)iter.next();
         ZipEntry zipEntry = (ZipEntry)childNode.getValue();
         FuseDirEnt dirEntry = new FuseDirEnt();
         dirEntries[i] = dirEntry;
         dirEntry.name = childNode.getName();
         dirEntry.mode = zipEntry.isDirectory()? FuseFtype.TYPE_DIR : FuseFtype.TYPE_FILE;
      }

      return dirEntries;
   }

   public void link(String from, String to) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void mkdir(String path, int mode) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void mknod(String path, int mode, int rdev) throws FuseException
   {
	      ActiveRDMA client = null;
	      
	      try {
	    	  client = new Client(server);
	      }
	      catch(Exception e){
	    	  System.out.println(e);
	      }
	      System.out.println("Initiated connection to server");
	      DFS dfs;

	      dfs = new DFS_RDMA(client);
	      int inode = dfs.lookup(path);
	      System.out.println("mknod : lookup done");
	  if (inode!=0) {
		  throw new FuseException("File Exists").initErrno(FuseException.EEXIST);
	  }
	  
	  else {
		  System.out.println("mknod : About to create file");
		 inode =  dfs.create(path);
	  }
	  System.out.println("mknod : everything done");
	  //return inode;
   }

   public void open(String path, int flags) throws FuseException
   {
      ZipEntry entry = getFileZipEntry(path);

/*      if (flags == O_WRONLY || flags == O_RDWR)
         throw new FuseException("Read Only").initErrno(FuseException.EACCES);*/
   }

   public void rename(String from, String to) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void rmdir(String path) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public FuseStatfs statfs() throws FuseException
   {
      return statfs;
   }

   public void symlink(String from, String to) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void truncate(String path, long size) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void unlink(String path) throws FuseException
   {
      throw new FuseException("Read Only").initErrno(FuseException.EACCES);
   }

   public void utime(String path, int atime, int mtime) throws FuseException
   {
      // noop
   }

   public String readlink(String path) throws FuseException
   {
      throw new FuseException("Not a link").initErrno(FuseException.ENOENT);
   }

   public void write(String path, ByteBuffer buf, long offset) throws FuseException
   {
      // noop
	      ActiveRDMA client = null;
	      
	      try {
	    	  client = new Client(server);
	      }
	      catch(Exception e){
	    	  System.out.println(e);
	      }
	      
	      DFS dfs;

	      dfs = new DFS_RDMA(client);
	      int inode = dfs.lookup(path);
	      int[] buffer;
	      IntBuffer intBuffer;
	      
	      //if() error checking code here
	      if(inode != 0){
	    	  intBuffer = buf.asIntBuffer();
		      buffer = new int[buf.capacity()/4];
		      intBuffer.get(buffer);
		      
		      dfs.put(inode, buffer, (int)offset, buf.capacity()/4);
	      }
	      //How to return errors?
	      
	      
   }

   public void read(String path, ByteBuffer buf, long offset) throws FuseException
   {
      /*ZipEntry zipEntry = getFileZipEntry(path);
      ZipEntryDataReader reader = zipFileDataReader.getZipEntryDataReader(zipEntry, offset, buf.capacity());

      reader.read(buf, offset);*/
      
      ActiveRDMA client = null;
      
      try {
    	  client = new Client(server);
      }
      catch(Exception e){
    	  System.out.println(e);
      }
      
      DFS dfs;

      dfs = new DFS_RDMA(client);
      int inode = dfs.lookup(path);
      IntBuffer intBuffer;
      int[] buffer;
      if(inode != 0){
          buffer = new int[(buf.capacity()/4)];
          dfs.get(inode, buffer, (int)offset, (buf.capacity()/4)); 
          intBuffer = buf.asIntBuffer();
          intBuffer.put(buffer);
      }
      //How to return errors?

   }

   public void release(String path, int flags) throws FuseException
   {
      ZipEntry zipEntry = getFileZipEntry(path);
      zipFileDataReader.releaseZipEntryDataReader(zipEntry);
   }


   //
   // private methods

   private ZipEntry getFileZipEntry(String path) throws FuseException
   {
      Node node = tree.lookupNode(path);
      ZipEntry entry;
      if (node == null || (entry = (ZipEntry)node.getValue()) == null)
         throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

      if (entry.isDirectory())
         throw new FuseException("Not A File").initErrno(FuseException.ENOENT);

      return entry;
   }

   private ZipEntry getDirectoryZipEntry(String path) throws FuseException
   {
      Node node = tree.lookupNode(path);
      ZipEntry entry;
      if (node == null || (entry = (ZipEntry)node.getValue()) == null)
         throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

      if (!entry.isDirectory())
         throw new FuseException("Not A Directory").initErrno(FuseException.ENOENT);

      return entry;
   }



   //
   // Java entry point

   public static void main(String[] args)
   {
      

      String fuseArgs[] = new String[args.length];
      System.arraycopy(args, 0, fuseArgs, 0, fuseArgs.length);
      //File zipFile = new File(args[args.length - 1]);

      try
      {
         FuseMount.mount(fuseArgs, new ARFilesystem());
      }
      catch (Exception e)
      {
         e.printStackTrace();
      }
   }
}
