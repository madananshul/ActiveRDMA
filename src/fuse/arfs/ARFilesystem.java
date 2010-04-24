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

import java.io.*;
import java.nio.*;
import java.util.Collection;
import java.util.Enumeration;
import java.util.Iterator;

import org.apache.commons.logging.LogFactory;
import org.apache.commons.logging.Log;

import client.Client;

import common.ActiveRDMA;


public class ARFilesystem implements Filesystem3
{
   private static final Log log = LogFactory.getLog(ARFilesystem.class);

   private static final int blockSize = 4096;
   
   final String server = "localhost";

   private FuseStatfs statfs;

   private DFS dfs;

   final String dir_prefix = "###___DIR___###";

   static class OpenFileHandle
   {
       public int inode;
       public int length;
       public OpenFileHandle (int i, int l) { inode = i; length = l; }
   }

   public ARFilesystem() throws Exception
   {
      statfs = new FuseStatfs();
      statfs.blocks = 0;
      statfs.blockSize = blockSize;
      statfs.blocksFree = 0;
      statfs.files = 0;
      statfs.filesFree = 0;
      statfs.namelen = 256;
      
      ActiveRDMA client = null;
	  try {
	   	  client = new Client(server);
	  }
	  catch(Exception e){
	   	  System.out.println(e);
	  }
	   
	  dfs = new DFS_Active(client);
	   
      dfs.create(dir_prefix + "/");
   }
   

   public int fsync(String path, Object fh, boolean isDatasync) throws FuseException
   {
	   return 0;
   }
   
   public int flush(String path, Object fh) throws FuseException
   {
	   return 0;
   }
   
   
   public int chmod(String path, int mode) throws FuseException
   {
	   return 0;
   }

   public int chown(String path, int uid, int gid) throws FuseException
   {
	   return 0;
   }

   public int getattr(String path, FuseGetattrSetter getattrSetter) throws FuseException
   {
       boolean dir = false;
	   int inode = dfs.lookup(path);
	   if (inode == 0 || dfs.getLen(inode) == -1)
       {
           inode = dfs.lookup(dir_prefix + path);
           dir = true;
       }
       if (inode == 0 || dfs.getLen(inode) == -1)
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

       int len = dfs.getLen(inode); // FILE DELETION HACK: size of -1 == deleted file. (world-class filesystem here!)
       if (len == -1)
           throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	  
      FuseStat stat = new FuseStat();

      if(dir)
    	  stat.mode = FuseFtype.TYPE_DIR | 0755;
      else 
    	  stat.mode = FuseFtype.TYPE_FILE | 0755;

      stat.nlink = 1;
      stat.uid = 0;
      stat.gid = 0;
      stat.size = len;
      stat.atime = stat.mtime = stat.ctime = 0;//(int)System.currentTimeMillis();
      stat.blocks = (int) ((stat.size + 4095L) / 4096L);
      getattrSetter.set(inode, stat.mode, stat.nlink, stat.uid, stat.gid, 0, stat.size, stat.blocks, stat.atime, stat.mtime, stat.ctime);

      return 0;
   }

   public int getdir(String path, FuseDirFiller dirFiller) throws FuseException
   {
	   int inode = dfs.lookup(dir_prefix + path);
	   if (inode == 0) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }

       int len = dfs.getLen(inode);
       if (len == -1)
           throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);

       byte[] buf = new byte[len];
       dfs.get(inode, buf, 0, len);

       try
       {
           ByteArrayInputStream str = new ByteArrayInputStream(buf);
           DataInputStream ds = new DataInputStream(str);
           while (ds.available() > 0)
           {
               int fInode = ds.readInt();
               int nameLen = ds.readUnsignedByte();
               byte[] name = new byte[nameLen];
               ds.read(name);
               String nameS = new String(name);
               boolean dir = false;

               if (dfs.getLen(fInode) == -1) continue; // deleted file

               if (nameS.startsWith(dir_prefix))
               {
                   dir = true;
                   nameS = nameS.substring(dir_prefix.length());
               }

               dirFiller.add(nameS, fInode, dir ? FuseFtype.TYPE_DIR  : FuseFtype.TYPE_FILE);
           }
       } catch (IOException e) {
           System.out.println("Malformed directory: exception!");
           e.printStackTrace();
           return 0;
       }

       return 0;
   }

   void appendDirEntry(int dirInode, String name, int inode) throws FuseException
   {
       byte[] dirEntry = null;
       try
       {
           ByteArrayOutputStream oub = new ByteArrayOutputStream();
           DataOutputStream out = new DataOutputStream(oub);

           byte[] nameB = name.getBytes();
           if (nameB.length > 255) throw new FuseException("Name Too Long").initErrno(FuseException.ENAMETOOLONG);

           out.writeInt(inode);
           out.writeByte(nameB.length);
           out.write(nameB, 0, nameB.length);

           dirEntry = oub.toByteArray();
       } catch (IOException e) {
       }

       int len = dfs.getLen(dirInode);
       dfs.setLen(dirInode, len + dirEntry.length);
       dfs.put(dirInode, dirEntry, len, dirEntry.length);
   }

   String dirname(String path)
   {
       int idx = path.lastIndexOf('/');
       if (idx == -1 || idx == 0)
           return new String("/");
       else
           return path.substring(0, idx);
   }

   String basename(String path)
   {
       int idx = path.lastIndexOf('/');
       if (idx == -1)
           return path;
       else
           return path.substring(idx + 1);
   }

   public int link(String from, String to) throws FuseException
   {
       int inode = dfs.lookup(from);
       if (inode == 0)
           throw new FuseException("Entry Not Found").initErrno(FuseException.ENOENT);

       int dirInode = dfs.lookup(dir_prefix + dirname(to));
       if (dirInode == 0)
           throw new FuseException("Destination Dir Not Found").initErrno(FuseException.ENOENT);

       appendDirEntry(dirInode, basename(to), inode);

	   return 0;
   }

   public int mkdir(String path, int mode) throws FuseException
   {
       int fileInode = dfs.lookup(path);
       if (fileInode != 0 && dfs.getLen(fileInode) != -1)
           throw new FuseException("Entry Exists as File").initErrno(FuseException.EEXIST);

       int prevInode = dfs.lookup(dir_prefix + path);
       if (prevInode != 0)
       {
           if (dfs.getLen(prevInode) != -1)
               throw new FuseException("Entry Exists as Dir").initErrno(FuseException.EEXIST);

           dfs.setLen(prevInode, 0);
           return 0;
       }

       int parent_inode = dfs.lookup(dir_prefix + dirname(path));
       if (parent_inode == 0)
           throw new FuseException("Entry Not Found").initErrno(FuseException.ENOENT);

       int inode = dfs.create(dir_prefix + path);
       appendDirEntry(parent_inode, basename(path), inode);
       return 0;
   }

   public int mknod(String path, int mode, int rdev) throws FuseException
   {
       int dirInode = dfs.lookup(dir_prefix + path);
       if (dirInode != 0 && dfs.getLen(dirInode) != -1)
           throw new FuseException("Entry Exists").initErrno(FuseException.EEXIST);

       int prevInode = dfs.lookup(path);
       if (prevInode != 0)
       {
           if (dfs.getLen(prevInode) != -1)
               throw new FuseException("Entry Exists").initErrno(FuseException.EEXIST);

           // already existed: just set length to 0 and return
           dfs.setLen(prevInode, 0);
           return 0;
       }

       int parent_inode = dfs.lookup(dir_prefix + dirname(path));
       if (parent_inode == 0)
           throw new FuseException("Entry Not Found").initErrno(FuseException.ENOENT);

       int inode = dfs.create(path);
       appendDirEntry(parent_inode, basename(path), inode);
       return 0;
   }

   public int open(String path, int flags, FuseOpenSetter openSetter) throws FuseException
   {
	   int inode = dfs.lookup(path);
       int len = -1;
       if (inode != 0) len = dfs.getLen(inode);

	   if (inode == 0 || len == -1) {
		   throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	   }
	   else {
		   openSetter.setFh(new OpenFileHandle(inode, len));
	   }
	  return 0;
   }

   public int rename(String from, String to) throws FuseException
   {
       link(from, to);
       unlink(from);
       return 0;
   }

   public int rmdir(String path) throws FuseException
   {
       unlink(dir_prefix + path);
       return 0;
   }

   public int statfs(FuseStatfsSetter statfsSetter) throws FuseException
   {
	   statfsSetter.set(statfs.blockSize, statfs.blocks, statfs.blocksFree, statfs.blocksAvail, statfs.files, statfs.filesFree, statfs.namelen);
	   
      return 0;
   }

   public int symlink(String from, String to) throws FuseException
   {
       throw new FuseException("Symlinks not supported").initErrno(FuseException.EACCES);
   }

   public int truncate(String path, long size) throws FuseException
   {
	   int inode = dfs.lookup(path);
       if (inode == 0)
           throw new FuseException("Entry Not Found").initErrno(FuseException.ENOENT);
           
	   dfs.setLen(inode, (int)size);
	   return 0;
   }

   public int unlink(String path) throws FuseException
   {
       int fInode = dfs.lookup(path);
       if (fInode == 0)
           throw new FuseException("Entry Not Found").initErrno(FuseException.ENOENT);

       dfs.setLen(fInode, -1);

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
       OpenFileHandle f = (OpenFileHandle)fh;

	      if (f == null)
          {
	    	  int inode = dfs.lookup(path);
              int len = dfs.getLen(inode);
              f = new OpenFileHandle(inode, len);
          }

	      if(f.inode != 0){
              if (buf.capacity() + offset > f.length)
              {
                  f.length = (int)(offset + buf.capacity());
                  dfs.setLen(f.inode, f.length);
              }

              byte[] bytebuf = new byte[buf.capacity()];

              buf.get(bytebuf);

              dfs.put(f.inode, bytebuf, (int)offset, buf.capacity());
		      
		      return 0;
	      }
	      throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
	      
   }

   public int read(String path, Object fh, ByteBuffer buf, long offset) throws FuseException
   {
       OpenFileHandle f = (OpenFileHandle)fh;
	   if(f == null)
       {
	   	  int inode = dfs.lookup(path);
          int fLen = dfs.getLen(inode);
          f = new OpenFileHandle(inode, fLen);
       }
	      
      if(f.inode != 0){
          int len = buf.capacity();
          if (offset > f.length) offset = f.length;
          if (len + (int)offset > f.length) len = f.length - (int)offset;

          byte[] bytebuf = new byte[len];
          
          if (len > 0)
              dfs.get(f.inode, bytebuf, (int)offset, len);

          buf.put(bytebuf);

          return 0;
      }
      throw new FuseException("No Such Entry").initErrno(FuseException.ENOENT);
   }

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

      try
      {
         FuseMount.mount(fuseArgs, new ARFilesystem(), null /* log */);
      }
      catch (Exception e)
      {
         e.printStackTrace();
      }
   }
}
