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
import fuse.compat.Filesystem2;
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


public class ARFilesystem implements Filesystem2
{
   private static final Log log = LogFactory.getLog(ARFilesystem.class);

   private static final int blockSize = 512;
   
   final String server = "localhost";

   //private ZipFile zipFile;
   //private long zipFileTime;
   private ZipEntry rootEntry;
   private Tree tree;
   private FuseStatfs statfs;

   //private ZipFileDataReader zipFileDataReader;
   
   private DFS dfs;


   public ARFilesystem() throws IOException
   {
      /*rootEntry = new ZipEntry("")
      {
         public boolean isDirectory()
         {
            return true;
         }
      };
      //rootEntry.setTime(zipFileTime);
      rootEntry.setSize(0);

      tree = new Tree();
      tree.addNode(rootEntry.getName(), rootEntry);*/

      statfs = new FuseStatfs();
      statfs.blocks = 0;
      statfs.blockSize = blockSize;
      statfs.blocksFree = 0;
      statfs.files = 0;
      statfs.filesFree = 0;
      statfs.namelen = 2048;
      
      ActiveRDMA client = null;
	   try {
	   	  client = new Client(server);
	   }
	   catch(Exception e){
	   	  System.out.println(e);
	   }
	   
	   dfs = new DFS_RDMA(client);
	   dfs.create("/");
   }

   

   // CHANGE-22: new operation (Synchronize file contents),
   //            isDatasync indicates that only the user data should be flushed, not the meta data
   public void fsync(String path, long fh, boolean isDatasync) throws FuseException
   {
	   
   }
   
   // CHANGE-22: new operation (called on every filehandle close)
   public void flush(String path, long fh) throws FuseException
   {
	   
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
        
	   int inode = dfs.lookup(path);
	   System.out.println("getattr : for path " + path+ " inode is " + inode);
	   if (inode == 0) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }

      FuseStat stat = new FuseStat();

      //stat.mode = entry.isDirectory() ? FuseFtype.TYPE_DIR | 0755 : FuseFtype.TYPE_FILE | 0644;
      stat.mode = FuseFtype.TYPE_FILE | 0755;
      stat.nlink = 1;
      stat.uid = 0;
      stat.gid = 0;
      stat.size = dfs.getLen(inode);
      //stat.atime = stat.mtime = stat.ctime = (int) (entry.getTime() / 1000L);
      stat.atime = stat.mtime = stat.ctime = 0;//(int)System.currentTimeMillis();
      stat.blocks = (int) ((stat.size + 511L) / 512L);

      return stat;
   }

   // CHANGE-22: FuseDirEnt.inode added
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
	      	      
	      int inode = dfs.lookup(path);
	      if (inode!=0) {
	    	  throw new FuseException("File Exists").initErrno(FuseException.EEXIST);
	      }
	  
	      else {
	    	  inode =  dfs.create(path);
	    	  System.out.println("mknod : Creating file "+path+" with inode no. "+inode);
	      }
	      //return inode;
   }

   public long open(String path, int flags) throws FuseException
   {
	   /*
      if (flags == O_WRONLY || flags == O_RDWR)
         throw new FuseException("Read Only").initErrno(FuseException.EACCES);*/
	   int inode = dfs.lookup(path);
	   if (inode == 0) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }
	  return inode;
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
	   int inode = dfs.lookup(path);
	   dfs.setLen(inode, (int)size);
	      
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

   // isWritepage indicates that write was caused by a writepage
   public void write(String path, long fh, boolean isWritepage, ByteBuffer buf, long offset) throws FuseException
   {
	      
	      int inode = 0;
	      if(fh == 0){
	    	  inode = dfs.lookup(path);
	      }
	      else 
	    	  inode = (int)fh;
	      
	      int[] buffer;
	      IntBuffer intBuffer;
	      
	      if(inode != 0){
	    	  intBuffer = buf.asIntBuffer();
		      buffer = new int[intBuffer.capacity()];
		      intBuffer.get(buffer);
		      
		      dfs.put(inode, buffer, (int)offset, intBuffer.capacity());
	      }
	      //How to return errors?
	      
	      
   }

   public void read(String path, long fh, ByteBuffer buf, long offset) throws FuseException
   {
      
	   int inode = 0;
	   if(fh==0){
	      inode = dfs.lookup(path);
	   }
	   else 
	   	  inode = (int)fh;
	      
      IntBuffer intBuffer;
      int[] buffer;
      if(inode != 0){
    	  intBuffer = buf.asIntBuffer();
          buffer = new int[intBuffer.capacity()];
          dfs.get(inode, buffer, (int)offset, intBuffer.capacity()); 
          intBuffer.put(buffer);
      }
      //How to return errors?

   }

   // CHANGE-22: (called when last filehandle is closed), fh is filehandle passed from open
   public void release(String path, long fh, int flags) throws FuseException
   {
      /*ZipEntry zipEntry = getFileZipEntry(path);
      zipFileDataReader.releaseZipEntryDataReader(zipEntry);*/
	  
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
