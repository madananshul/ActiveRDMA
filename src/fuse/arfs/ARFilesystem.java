/**
 *   FUSE-J: Java bindings for FUSE (Filesystem in Userspace by Miklos Szeredi (mszeredi@inf.bme.hu))
 *
 *   Copyright (C) 2003 Peter Levart (peter@select-tech.si)
 *
 *   This program can be distributed under the terms of the GNU LGPL.
 *   See the file COPYING.LIB
 */

/*
 * ARFilesystem : Copyright 15712 Active RDMA Project
 * 
 */

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
import fuse.arfs.util.Node;
import fuse.arfs.util.Tree;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
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


public class ARFilesystem implements Filesystem3
{
   private static final Log log = LogFactory.getLog(ARFilesystem.class);

   private static final int blockSize = 512;
   
   final String server = "localhost";

   //private ZipFile zipFile;
   //private long zipFileTime;
   //private ZipEntry rootEntry;
   private Tree tree;
   private FuseStatfs statfs;

   //private ZipFileDataReader zipFileDataReader;
   
   private DFS dfs;


   public ARFilesystem() throws IOException
   {
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
   public int fsync(String path, Object fh, boolean isDatasync) throws FuseException
   {
	   return 0;
   }
   
   // CHANGE-22: new operation (called on every filehandle close)
   public int flush(String path, Object fh) throws FuseException
   {
	   return 0;
   }
   
   
   public int chmod(String path, int mode) throws FuseException
   {
      //throw new FuseException("Read Only").initErrno(FuseException.EACCES);
	   return 0;
   }

   public int chown(String path, int uid, int gid) throws FuseException
   {
      //throw new FuseException("Read Only").initErrno(FuseException.EACCES);
	   return 0;
   }

   public int getattr(String path, FuseGetattrSetter getattrSetter) throws FuseException
   {
        
	   int inode = dfs.lookup(path);
	   System.out.println("getattr : for path " + path+ " inode is " + inode);
	   if (inode == 0) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }

	  
      FuseStat stat = new FuseStat();

      if(path.equals("/"))
    	  stat.mode = FuseFtype.TYPE_DIR | 0755;
      else 
    	  stat.mode = FuseFtype.TYPE_FILE | 0755;
      stat.nlink = 1;
      stat.uid = 0;
      stat.gid = 0;
      stat.size = 4*dfs.getLen(inode);
      //stat.atime = stat.mtime = stat.ctime = (int) (entry.getTime() / 1000L);
      stat.atime = stat.mtime = stat.ctime = 0;//(int)System.currentTimeMillis();
      stat.blocks = (int) ((stat.size + 511L) / 512L);
      getattrSetter.set(inode, stat.mode, stat.nlink, stat.uid, stat.gid, 0, stat.size, stat.blocks, stat.atime, stat.mtime, stat.ctime);

      return 0;
   }

   // CHANGE-22: FuseDirEnt.inode added
   public int getdir(String path, FuseDirFiller dirFiller) throws FuseException
   {
	   int inode = dfs.lookup(path);
	   if (inode == 0) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }

      //if (!entry.isDirectory())
      //   throw new FuseException("Not A Directory").initErrno(FuseException.ENOTDIR);

	  // FuseDirEnt[] dirEntries = new FuseDirEnt[0];

      //int i = 0;
      //for (Iterator iter = children.iterator(); iter.hasNext(); i++)
      //{
         
         //ZipEntry zipEntry = (ZipEntry)childNode.getValue();
         //FuseDirEnt dirEntry = new FuseDirEnt();
         //dirEntries[i] = dirEntry;
         //dirEntry.name = childNode.getName();
         //dirEntry.mode = zipEntry.isDirectory()? FuseFtype.TYPE_DIR : FuseFtype.TYPE_FILE;
      //}

      //return dirEntries;
	   return 0;
   }

   public int link(String from, String to) throws FuseException
   {
      //throw new FuseException("Read Only").initErrno(FuseException.EACCES);
	   return 0;
   }

   public int mkdir(String path, int mode) throws FuseException
   {
      return 0;
   }

   public int mknod(String path, int mode, int rdev) throws FuseException
   {
	      	      
	      int inode = dfs.lookup(path);
	      if (inode!=0) {
	    	  throw new FuseException("File Exists").initErrno(FuseException.EEXIST);
	      }
	  
	      else {
	    	  inode =  dfs.create(path);
	    	  System.out.println("mknod : Creating file "+path+" with inode no. "+inode);
	      }
	      return 0;
   }

   public int open(String path, int flags, FuseOpenSetter openSetter) throws FuseException
   {
	   /*
      if (flags == O_WRONLY || flags == O_RDWR)
         throw new FuseException("Read Only").initErrno(FuseException.EACCES);*/
	   int inode = dfs.lookup(path);
	   if (inode == 0) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }
	   else {
		   openSetter.setFh(inode);
	   }
	  return 0;
   }

   public int rename(String from, String to) throws FuseException
   {
      return 0;
   }

   public int rmdir(String path) throws FuseException
   {
      return 0;
   }

   public int statfs(FuseStatfsSetter statfsSetter) throws FuseException
   {
	   statfsSetter.set(statfs.blockSize, statfs.blocks, statfs.blocksFree, statfs.blocksAvail, statfs.files, statfs.filesFree, statfs.namelen);
	   
      return 0;
   }

   public int symlink(String from, String to) throws FuseException
   {
	   return 0;
   }

   public int truncate(String path, long size) throws FuseException
   {
	   int inode = dfs.lookup(path);
	   dfs.setLen(inode, (int)size);
	   return 0;
   }

   public int unlink(String path) throws FuseException
   {
      return 0;
   }

   public int utime(String path, int atime, int mtime) throws FuseException
   {
	   return 0;
   }

   public int readlink(String path, CharBuffer link) throws FuseException
   {
      throw new FuseException("Not a link").initErrno(FuseException.ENOENT);
   }

   // isWritepage indicates that write was caused by a writepage
   public int write(String path, Object fh, boolean isWritepage, ByteBuffer buf, long offset) throws FuseException
   {
	      int inode = 0;
	      if(fh.equals(new Integer(inode))){
	    	  inode = dfs.lookup(path);
	      }
	      else 
	    	  inode = ((Integer)fh).intValue();
	      
	      int[] buffer;
	      IntBuffer intBuffer;
	      
	      if(inode != 0){
	    	  //intBuffer = ((ByteBuffer) buf.rewind()).asIntBuffer();
	    	  intBuffer = buf.asIntBuffer();
	    	  System.out.println("Writing " + buf.capacity() + " bytes and " + intBuffer.capacity() + " ints.");
	    	  int pad = ((buf.capacity()%4) != 0)?1:0;
		      //buffer = new int[intBuffer.capacity()];
		      //intBuffer.get(buffer, 0 , buffer.length);
	    	  //buffer = intBuffer.array();
		      
		      /*int rem = buf.capacity()%4;
		      for (int i = 0; i <= rem; i++) {
		            int shift = (rem - i) * 8;
		            buffer[intBuffer.capacity()] += (buf.get(i) & 0x000000FF) << shift;
		      }*/
		      
		      //Nasty Hack that writes equivalent amount of data, but random junk
		      buffer = new int[intBuffer.capacity() + pad];
		      for(int i=0;i<intBuffer.capacity();i++)
	        	  System.out.println("Writing int " + buffer[i] + " bytes" + buf.get(i*4) + " " + buf.get(i*4+1) + " " + buf.get(i*4+2) + " " + buf.get(i*4+3));
	          
		      
		      /*System.out.println("Finally Writing " + buffer.length + " ints.");*/
		      dfs.setLen(inode, intBuffer.capacity() + pad);
		      //dfs.put(inode, buffer, (int)offset, intBuffer.capacity());
		      return 0;
	      }
	      throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	      
   }

   //TBD - Problem in translation code. Fix this.
   public int read(String path, Object fh, ByteBuffer buf, long offset) throws FuseException
   {
	   int fLen;
	   int inode = 0;
	   if(fh.equals(new Integer(inode))){
	   	  inode = dfs.lookup(path);
	   }
	   else 
	   	  inode = ((Integer)fh).intValue();
	      
	  fLen = dfs.getLen(inode);
	  //fLen = 2; 
	   
      IntBuffer intBuffer;
      int[] buffer;
      if(inode != 0){
    	  intBuffer = buf.asIntBuffer();
          buffer = new int[fLen];
          System.out.println("Reading " + fLen + " ints.");
          //dfs.get(inode, buffer, (int)offset, fLen);
          /*for(int i=0;i<fLen;i++)
        	  System.out.println("Before int buffer put, Reading int " + buffer[i] + " bytes" + buf.get(i*4) + " " + buf.get(i*4+1) + " " + buf.get(i*4+2) + " " + buf.get(i*4+3));*/
          intBuffer.put(buffer);
          /*for(int i=0;i<fLen;i++)
        	  System.out.println("After int buffer put, Reading int " + buffer[i] + " bytes" + buf.get(i*4) + " " + buf.get(i*4+1) + " " + buf.get(i*4+2) + " " + buf.get(i*4+3));*/
          return 0;
      }
      throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

   }

   // CHANGE-22: (called when last filehandle is closed), fh is filehandle passed from open
   public int release(String path, Object fh, int flags) throws FuseException
   {
      
	  return 0;
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
         FuseMount.mount(fuseArgs, new ARFilesystem(), log);
      }
      catch (Exception e)
      {
         e.printStackTrace();
      }
   }
}
