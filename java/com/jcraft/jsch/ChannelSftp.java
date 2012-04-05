/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2002-2012 ymnk, JCraft,Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in 
     the documentation and/or other materials provided with the distribution.

  3. The names of the authors may not be used to endorse or promote products
     derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JCRAFT,
INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package com.jcraft.jsch;

import java.io.*;

import java.util.Vector;

public class ChannelSftp extends ChannelSession{

  static private final int LOCAL_MAXIMUM_PACKET_SIZE=32*1024;
  static private final int LOCAL_WINDOW_SIZE_MAX=(64*LOCAL_MAXIMUM_PACKET_SIZE);

  private static final byte SSH_FXP_INIT=               1;
  private static final byte SSH_FXP_VERSION=            2;
  private static final byte SSH_FXP_OPEN=               3;
  private static final byte SSH_FXP_CLOSE=              4;
  private static final byte SSH_FXP_READ=               5;
  private static final byte SSH_FXP_WRITE=              6;
  private static final byte SSH_FXP_LSTAT=              7;
  private static final byte SSH_FXP_FSTAT=              8;
  private static final byte SSH_FXP_SETSTAT=            9;
  private static final byte SSH_FXP_FSETSTAT=          10;
  private static final byte SSH_FXP_OPENDIR=           11;
  private static final byte SSH_FXP_READDIR=           12;
  private static final byte SSH_FXP_REMOVE=            13;
  private static final byte SSH_FXP_MKDIR=             14;
  private static final byte SSH_FXP_RMDIR=             15;
  private static final byte SSH_FXP_REALPATH=          16;
  private static final byte SSH_FXP_STAT=              17;
  private static final byte SSH_FXP_RENAME=            18;
  private static final byte SSH_FXP_READLINK=          19;
  private static final byte SSH_FXP_SYMLINK=           20;
  private static final byte SSH_FXP_STATUS=           101;
  private static final byte SSH_FXP_HANDLE=           102;
  private static final byte SSH_FXP_DATA=             103;
  private static final byte SSH_FXP_NAME=             104;
  private static final byte SSH_FXP_ATTRS=            105;
  private static final byte SSH_FXP_EXTENDED=         (byte)200;
  private static final byte SSH_FXP_EXTENDED_REPLY=   (byte)201;

  // pflags
  private static final int SSH_FXF_READ=           0x00000001;
  private static final int SSH_FXF_WRITE=          0x00000002;
  private static final int SSH_FXF_APPEND=         0x00000004;
  private static final int SSH_FXF_CREAT=          0x00000008;
  private static final int SSH_FXF_TRUNC=          0x00000010;
  private static final int SSH_FXF_EXCL=           0x00000020;

  private static final int SSH_FILEXFER_ATTR_SIZE=         0x00000001;
  private static final int SSH_FILEXFER_ATTR_UIDGID=       0x00000002;
  private static final int SSH_FILEXFER_ATTR_PERMISSIONS=  0x00000004;
  private static final int SSH_FILEXFER_ATTR_ACMODTIME=    0x00000008;
  private static final int SSH_FILEXFER_ATTR_EXTENDED=     0x80000000;

  public static final int SSH_FX_OK=                            0;
  public static final int SSH_FX_EOF=                           1;
  public static final int SSH_FX_NO_SUCH_FILE=                  2;
  public static final int SSH_FX_PERMISSION_DENIED=             3;
  public static final int SSH_FX_FAILURE=                       4;
  public static final int SSH_FX_BAD_MESSAGE=                   5;
  public static final int SSH_FX_NO_CONNECTION=                 6;
  public static final int SSH_FX_CONNECTION_LOST=               7;
  public static final int SSH_FX_OP_UNSUPPORTED=                8;
/*
   SSH_FX_OK
      Indicates successful completion of the operation.
   SSH_FX_EOF
     indicates end-of-file condition; for SSH_FX_READ it means that no
       more data is available in the file, and for SSH_FX_READDIR it
      indicates that no more files are contained in the directory.
   SSH_FX_NO_SUCH_FILE
      is returned when a reference is made to a file which should exist
      but doesn't.
   SSH_FX_PERMISSION_DENIED
      is returned when the authenticated user does not have sufficient
      permissions to perform the operation.
   SSH_FX_FAILURE
      is a generic catch-all error message; it should be returned if an
      error occurs for which there is no more specific error code
      defined.
   SSH_FX_BAD_MESSAGE
      may be returned if a badly formatted packet or protocol
      incompatibility is detected.
   SSH_FX_NO_CONNECTION
      is a pseudo-error which indicates that the client has no
      connection to the server (it can only be generated locally by the
      client, and MUST NOT be returned by servers).
   SSH_FX_CONNECTION_LOST
      is a pseudo-error which indicates that the connection to the
      server has been lost (it can only be generated locally by the
      client, and MUST NOT be returned by servers).
   SSH_FX_OP_UNSUPPORTED
      indicates that an attempt was made to perform an operation which
      is not supported for the server (it may be generated locally by
      the client if e.g.  the version number exchange indicates that a
      required feature is not supported by the server, or it may be
      returned by the server if the server does not implement an
      operation).
*/
  private static final int MAX_MSG_LENGTH = 256* 1024;

  public static final int OVERWRITE=0;
  public static final int RESUME=1;
  public static final int APPEND=2;

  private boolean interactive=false;
  private int seq=1;
  private int[] ackid=new int[1];

  private Buffer buf;
  private Packet packet;

  // The followings will be used in file uploading.
  private Buffer obuf;
  private Packet opacket;

  private int client_version=3;
  private int server_version=3;
  private String version=String.valueOf(client_version);

  private java.util.Hashtable extensions=null;
  private InputStream io_in=null;

/*
10. Changes from previous protocol versions
  The SSH File Transfer Protocol has changed over time, before it's
   standardization.  The following is a description of the incompatible
   changes between different versions.
10.1 Changes between versions 3 and 2
   o  The SSH_FXP_READLINK and SSH_FXP_SYMLINK messages were added.
   o  The SSH_FXP_EXTENDED and SSH_FXP_EXTENDED_REPLY messages were added.
   o  The SSH_FXP_STATUS message was changed to include fields `error
      message' and `language tag'.
10.2 Changes between versions 2 and 1
   o  The SSH_FXP_RENAME message was added.
10.3 Changes between versions 1 and 0
   o  Implementation changes, no actual protocol changes.
*/

  private static final String file_separator=java.io.File.separator;
  private static final char file_separatorc=java.io.File.separatorChar;
  private static boolean fs_is_bs=(byte)java.io.File.separatorChar == '\\';

  private String cwd;
  private String home;
  private String lcwd;

  private static final String UTF8="UTF-8";
  private String fEncoding=UTF8;
  private boolean fEncoding_is_utf8=true;

  private RequestQueue rq = new RequestQueue(10);
  public void setBulkRequests(int bulk_requests) throws JSchException {
    if(bulk_requests>0) 
      rq = new RequestQueue(bulk_requests);
    else 
      throw new JSchException("setBulkRequests: "+ 
                              bulk_requests+" must be greater than 0.");
  }
  public int getBulkRequests(){
    return rq.size();
  }

  ChannelSftp(){
    super();
    setLocalWindowSizeMax(LOCAL_WINDOW_SIZE_MAX);
    setLocalWindowSize(LOCAL_WINDOW_SIZE_MAX);
    setLocalPacketSize(LOCAL_MAXIMUM_PACKET_SIZE);
  }

  void init(){
  }

  public void start() throws JSchException{
    try{

      PipedOutputStream pos=new PipedOutputStream();
      io.setOutputStream(pos);
      PipedInputStream pis=new MyPipedInputStream(pos, rmpsize);
      io.setInputStream(pis);

      io_in=io.in;

      if(io_in==null){
        throw new JSchException("channel is down");
      }

      Request request=new RequestSftp();
      request.request(getSession(), this);

      /*
      System.err.println("lmpsize: "+lmpsize);
      System.err.println("lwsize: "+lwsize);
      System.err.println("rmpsize: "+rmpsize);
      System.err.println("rwsize: "+rwsize);
      */

      buf=new Buffer(lmpsize);
      packet=new Packet(buf);

      obuf=new Buffer(rmpsize);
      opacket=new Packet(obuf);

      int i=0;
      int length;
      int type;
      byte[] str;

      // send SSH_FXP_INIT
      sendINIT();

      // receive SSH_FXP_VERSION
      Header header=new Header();
      header=header(buf, header);
      length=header.length;
      if(length > MAX_MSG_LENGTH){
        throw new SftpException(SSH_FX_FAILURE, 
                                "Received message is too long: " + length);
      }
      type=header.type;             // 2 -> SSH_FXP_VERSION
      server_version=header.rid;
      //System.err.println("SFTP protocol server-version="+server_version);
      if(length>0){
        extensions=new java.util.Hashtable();
        // extension data
        fill(buf, length);
        byte[] extension_name=null;
        byte[] extension_data=null;
        while(length>0){
          extension_name=buf.getString();
          length-=(4+extension_name.length);
          extension_data=buf.getString();
          length-=(4+extension_data.length);
          extensions.put(Util.byte2str(extension_name),
                         Util.byte2str(extension_data));
        }
      }

      lcwd=new File(".").getCanonicalPath();
    }
    catch(Exception e){
      //System.err.println(e);
      if(e instanceof JSchException) throw (JSchException)e;
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }
  }

  public void quit(){ disconnect();}
  public void exit(){ disconnect();}
  public void lcd(String path) throws SftpException{
    path=localAbsolutePath(path);
    if((new File(path)).isDirectory()){
      try{
	path=(new File(path)).getCanonicalPath();
      }
      catch(Exception e){}
      lcwd=path;
      return;
    }
    throw new SftpException(SSH_FX_NO_SUCH_FILE, "No such directory");
  }

  public void cd(String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);
      path=isUnique(path);

      byte[] str=_realpath(path);
      SftpATTRS attr=_stat(str);

      if((attr.getFlags()&SftpATTRS.SSH_FILEXFER_ATTR_PERMISSIONS)==0){
        throw new SftpException(SSH_FX_FAILURE, 
                                "Can't change directory: "+path);
      }
      if(!attr.isDir()){
        throw new SftpException(SSH_FX_FAILURE, 
                                "Can't change directory: "+path);
      }

      setCwd(Util.byte2str(str, fEncoding));
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void put(String src, String dst) throws SftpException{
    put(src, dst, null, OVERWRITE);
  }
  public void put(String src, String dst, int mode) throws SftpException{
    put(src, dst, null, mode);
  }
  public void put(String src, String dst, 
		  SftpProgressMonitor monitor) throws SftpException{
    put(src, dst, monitor, OVERWRITE);
  }
  public void put(String src, String dst, 
		  SftpProgressMonitor monitor, int mode) throws SftpException{

    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      src=localAbsolutePath(src);
      dst=remoteAbsolutePath(dst);

      Vector v=glob_remote(dst);
      int vsize=v.size();
      if(vsize!=1){
        if(vsize==0){
          if(isPattern(dst))
            throw new SftpException(SSH_FX_FAILURE, dst);
          else
            dst=Util.unquote(dst);
        }
        throw new SftpException(SSH_FX_FAILURE, v.toString());
      }
      else{
        dst=(String)(v.elementAt(0));
      }

      boolean isRemoteDir=isRemoteDir(dst);

      v=glob_local(src);
      vsize=v.size();

      StringBuffer dstsb=null;
      if(isRemoteDir){
        if(!dst.endsWith("/")){
	    dst+="/";
        }
        dstsb=new StringBuffer(dst);
      }
      else if(vsize>1){
        throw new SftpException(SSH_FX_FAILURE, 
                                "Copying multiple files, but the destination is missing or a file.");
      }

      for(int j=0; j<vsize; j++){
	String _src=(String)(v.elementAt(j));
	String _dst=null;
	if(isRemoteDir){
	  int i=_src.lastIndexOf(file_separatorc);
          if(fs_is_bs){
            int ii=_src.lastIndexOf('/');
            if(ii!=-1 && ii>i)
              i=ii; 
          }
	  if(i==-1) dstsb.append(_src);
	  else dstsb.append(_src.substring(i + 1));
          _dst=dstsb.toString();
          dstsb.delete(dst.length(), _dst.length());
	}
        else{
          _dst=dst;
        }
        //System.err.println("_dst "+_dst);

	long size_of_dst=0;
	if(mode==RESUME){
	  try{
	    SftpATTRS attr=_stat(_dst);
	    size_of_dst=attr.getSize();
	  }
	  catch(Exception eee){
	    //System.err.println(eee);
	  }
	  long size_of_src=new File(_src).length();
	  if(size_of_src<size_of_dst){
	    throw new SftpException(SSH_FX_FAILURE, 
                                    "failed to resume for "+_dst);
	  }
	  if(size_of_src==size_of_dst){
	    return;
	  }
	}

        if(monitor!=null){
 	  monitor.init(SftpProgressMonitor.PUT, _src, _dst,
		       (new File(_src)).length());
	  if(mode==RESUME){
	    monitor.count(size_of_dst);
	  }
        }
	FileInputStream fis=null;
	try{
	  fis=new FileInputStream(_src);
	  _put(fis, _dst, monitor, mode);
	}
	finally{
	  if(fis!=null) {
	    fis.close();
	  }
	}
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, e.toString(), (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, e.toString());
    }
  }
  public void put(InputStream src, String dst) throws SftpException{
    put(src, dst, null, OVERWRITE);
  }
  public void put(InputStream src, String dst, int mode) throws SftpException{
    put(src, dst, null, mode);
  }
  public void put(InputStream src, String dst, 
		  SftpProgressMonitor monitor) throws SftpException{
    put(src, dst, monitor, OVERWRITE);
  }
  public void put(InputStream src, String dst, 
		  SftpProgressMonitor monitor, int mode) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      dst=remoteAbsolutePath(dst);

      Vector v=glob_remote(dst);
      int vsize=v.size();
      if(vsize!=1){
        if(vsize==0){
          if(isPattern(dst))
            throw new SftpException(SSH_FX_FAILURE, dst);
          else
            dst=Util.unquote(dst);
        }
        throw new SftpException(SSH_FX_FAILURE, v.toString());
      }
      else{
        dst=(String)(v.elementAt(0));
      }

      if(isRemoteDir(dst)){
        throw new SftpException(SSH_FX_FAILURE, dst+" is a directory");
      }

      if(monitor!=null){
        monitor.init(SftpProgressMonitor.PUT, 
                     "-", dst,
                     SftpProgressMonitor.UNKNOWN_SIZE);
      }

      _put(src, dst, monitor, mode);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, e.toString(), (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, e.toString());
    }
  }

  public void _put(InputStream src, String dst, 
                   SftpProgressMonitor monitor, int mode) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      byte[] dstb=Util.str2byte(dst, fEncoding);
      long skip=0;
      if(mode==RESUME || mode==APPEND){
	try{
	  SftpATTRS attr=_stat(dstb);
	  skip=attr.getSize();
	}
	catch(Exception eee){
	  //System.err.println(eee);
	}
      }
      if(mode==RESUME && skip>0){
	long skipped=src.skip(skip);
	if(skipped<skip){
	  throw new SftpException(SSH_FX_FAILURE, "failed to resume for "+dst);
	}
      }

      if(mode==OVERWRITE){ sendOPENW(dstb); }
      else{ sendOPENA(dstb); }

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_STATUS && type!=SSH_FXP_HANDLE){
	throw new SftpException(SSH_FX_FAILURE, "invalid type="+type);
      }
      if(type==SSH_FXP_STATUS){
        int i=buf.getInt();
        throwStatusError(buf, i);
      }
      byte[] handle=buf.getString();         // handle
      byte[] data=null;

      boolean dontcopy=true;

      if(!dontcopy){  // This case will not work anymore.
        data=new byte[obuf.buffer.length
                      -(5+13+21+handle.length+Session.buffer_margin
                        )
        ];
      }

      long offset=0;
      if(mode==RESUME || mode==APPEND){
	offset+=skip;
      }

      int startid=seq;
      int ackcount=0;
      int _s=0;
      int _datalen=0;

      if(!dontcopy){  // This case will not work anymore.
        _datalen=data.length;
      }
      else{
        data=obuf.buffer;
        _s=5+13+21+handle.length;
        _datalen=obuf.buffer.length-_s-Session.buffer_margin;
      }

      int bulk_requests = rq.size();

      while(true){
        int nread=0;
        int count=0;
        int s=_s;
        int datalen=_datalen;

        do{
          nread=src.read(data, s, datalen);
          if(nread>0){
            s+=nread;
            datalen-=nread;
            count+=nread;
          }
        }
        while(datalen>0 && nread>0); 
        if(count<=0)break;

        int foo=count;
        while(foo>0){
          if((seq-1)==startid ||
             ((seq-startid)-ackcount)>=bulk_requests){
            while(((seq-startid)-ackcount)>=bulk_requests){
              if(this.rwsize>=foo) break;
              if(checkStatus(ackid, header)){
                int _ackid = ackid[0];
                if(startid>_ackid || _ackid>seq-1){
                  if(_ackid==seq){
                    System.err.println("ack error: startid="+startid+" seq="+seq+" _ackid="+_ackid);
                  } 
                  else{
                    throw new SftpException(SSH_FX_FAILURE, "ack error: startid="+startid+" seq="+seq+" _ackid="+_ackid);
                  }
                }
                ackcount++;
              }
              else{
                break;
              }
            }
          }
          foo-=sendWRITE(handle, offset, data, 0, foo);
        }
        offset+=count;
	if(monitor!=null && !monitor.count(count)){
          break;
	}
      }
      int _ackcount=seq-startid;
      while(_ackcount>ackcount){
        if(!checkStatus(null, header)){
          break;
        }
        ackcount++;
      }
      if(monitor!=null)monitor.end();
      _sendCLOSE(handle, header);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, e.toString(), (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, e.toString());
    }
  }

  public OutputStream put(String dst) throws SftpException{
    return put(dst, (SftpProgressMonitor)null, OVERWRITE);
  }
  public OutputStream put(String dst, final int mode) throws SftpException{
    return put(dst, (SftpProgressMonitor)null, mode);
  }
  public OutputStream put(String dst, final SftpProgressMonitor monitor, final int mode) throws SftpException{
    return put(dst, monitor, mode, 0);
  }
  public OutputStream put(String dst, final SftpProgressMonitor monitor, final int mode, long offset) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      dst=remoteAbsolutePath(dst);
      dst=isUnique(dst);

      if(isRemoteDir(dst)){
	throw new SftpException(SSH_FX_FAILURE, dst+" is a directory");
      }

      byte[] dstb=Util.str2byte(dst, fEncoding);

      long skip=0;
      if(mode==RESUME || mode==APPEND){
	try{
	  SftpATTRS attr=_stat(dstb);
	  skip=attr.getSize();
	}
	catch(Exception eee){
	  //System.err.println(eee);
	}
      }

      if(mode==OVERWRITE){ sendOPENW(dstb); }
      else{ sendOPENA(dstb); }

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_STATUS && type!=SSH_FXP_HANDLE){
	throw new SftpException(SSH_FX_FAILURE, "");
      }
      if(type==SSH_FXP_STATUS){
        int i=buf.getInt();
        throwStatusError(buf, i);
      }
      final byte[] handle=buf.getString();         // handle

      if(mode==RESUME || mode==APPEND){
	offset+=skip;
      }

      final long[] _offset=new long[1];
      _offset[0]=offset;
      OutputStream out = new OutputStream(){
        private boolean init=true;
        private boolean isClosed=false;
        private int[] ackid=new int[1];
        private int startid=0;
        private int _ackid=0;
        private int ackcount=0;
        private int writecount=0;
        private Header header=new Header();          

        public void write(byte[] d) throws java.io.IOException{
          write(d, 0, d.length);
        }

        public void write(byte[] d, int s, int len) throws java.io.IOException{
          if(init){
            startid=seq;
            _ackid=seq;
            init=false;
          }

          if(isClosed){
            throw new IOException("stream already closed");
          }

          try{
            int _len=len;
            while(_len>0){
              int sent=sendWRITE(handle, _offset[0], d, s, _len);
              writecount++;
              _offset[0]+=sent;
              s+=sent;
              _len-=sent;
              if((seq-1)==startid ||
                 io_in.available()>=1024){
                while(io_in.available()>0){
                  if(checkStatus(ackid, header)){
                    _ackid=ackid[0];
                    if(startid>_ackid || _ackid>seq-1){
                      throw new SftpException(SSH_FX_FAILURE, "");
                    }
                    ackcount++;
                  }
                  else{
                    break;
                  }
                }
              }
            }
    	    if(monitor!=null && !monitor.count(len)){
              close();
              throw new IOException("canceled");
	    }
          }
          catch(IOException e){ throw e; }
          catch(Exception e){ throw new IOException(e.toString());  }
        }

        byte[] _data=new byte[1];
        public void write(int foo) throws java.io.IOException{
          _data[0]=(byte)foo;
          write(_data, 0, 1);
        }

        public void flush() throws java.io.IOException{

          if(isClosed){
            throw new IOException("stream already closed");
          }

          if(!init){
            try{
              while(writecount>ackcount){
                if(!checkStatus(null, header)){
                  break;
                }
                ackcount++;
              }
            }
            catch(SftpException e){
              throw new IOException(e.toString());
            }
          }
        }

        public void close() throws java.io.IOException{
          if(isClosed){
            return;
          }
          flush();
          if(monitor!=null)monitor.end();
          try{ _sendCLOSE(handle, header); }
          catch(IOException e){ throw e; }
          catch(Exception e){
            throw new IOException(e.toString());
          }
          isClosed=true;
        }
      };
      return out;
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void get(String src, String dst) throws SftpException{
    get(src, dst, null, OVERWRITE);
  }
  public void get(String src, String dst,
		  SftpProgressMonitor monitor) throws SftpException{
    get(src, dst, monitor, OVERWRITE);
  }
  public void get(String src, String dst,
		  SftpProgressMonitor monitor, int mode) throws SftpException{
    // System.out.println("get: "+src+" "+dst);

    boolean _dstExist = false;
    String _dst=null;
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      src=remoteAbsolutePath(src);
      dst=localAbsolutePath(dst);

      Vector v=glob_remote(src);
      int vsize=v.size();
      if(vsize==0){
        throw new SftpException(SSH_FX_NO_SUCH_FILE, "No such file");
      }

      File dstFile=new File(dst);
      boolean isDstDir=dstFile.isDirectory();
      StringBuffer dstsb=null;
      if(isDstDir){
        if(!dst.endsWith(file_separator)){
          dst+=file_separator;
        }
        dstsb=new StringBuffer(dst);
      }
      else if(vsize>1){
        throw new SftpException(SSH_FX_FAILURE, 
                                "Copying multiple files, but destination is missing or a file.");
      }

      for(int j=0; j<vsize; j++){
	String _src=(String)(v.elementAt(j));
	SftpATTRS attr=_stat(_src);
        if(attr.isDir()){
          throw new SftpException(SSH_FX_FAILURE, 
                                  "not supported to get directory "+_src);
        } 

	_dst=null;
	if(isDstDir){
	  int i=_src.lastIndexOf('/');
	  if(i==-1) dstsb.append(_src);
	  else dstsb.append(_src.substring(i + 1));
          _dst=dstsb.toString();
          dstsb.delete(dst.length(), _dst.length());
	}
        else{
          _dst=dst;
        }

        File _dstFile=new File(_dst);
	if(mode==RESUME){
	  long size_of_src=attr.getSize();
	  long size_of_dst=_dstFile.length();
	  if(size_of_dst>size_of_src){
	    throw new SftpException(SSH_FX_FAILURE, 
                                    "failed to resume for "+_dst);
	  }
	  if(size_of_dst==size_of_src){
	    return;
	  }
	}

	if(monitor!=null){
	  monitor.init(SftpProgressMonitor.GET, _src, _dst, attr.getSize());
	  if(mode==RESUME){
	    monitor.count(_dstFile.length());
	  }
	}

        FileOutputStream fos=null;
        _dstExist = _dstFile.exists();
        try{
          if(mode==OVERWRITE){
            fos=new FileOutputStream(_dst);
          }
          else{
            fos=new FileOutputStream(_dst, true); // append
          }
          // System.err.println("_get: "+_src+", "+_dst);
          _get(_src, fos, monitor, mode, new File(_dst).length());
        }
        finally{
          if(fos!=null){
            fos.close();
          }
        }
      }
    }
    catch(Exception e){
      if(!_dstExist && _dst!=null){
        File _dstFile = new File(_dst);
        if(_dstFile.exists() && _dstFile.length()==0){
          _dstFile.delete();
        }
      }
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }
  public void get(String src, OutputStream dst) throws SftpException{
    get(src, dst, null, OVERWRITE, 0);
  }
  public void get(String src, OutputStream dst,
		  SftpProgressMonitor monitor) throws SftpException{
    get(src, dst, monitor, OVERWRITE, 0);
  }
  public void get(String src, OutputStream dst,
		   SftpProgressMonitor monitor, int mode, long skip) throws SftpException{
//System.err.println("get: "+src+", "+dst);
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      src=remoteAbsolutePath(src);
      src=isUnique(src);

      if(monitor!=null){
	SftpATTRS attr=_stat(src);
        monitor.init(SftpProgressMonitor.GET, src, "??", attr.getSize());
        if(mode==RESUME){
          monitor.count(skip);
        }
      }
      _get(src, dst, monitor, mode, skip);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  private void _get(String src, OutputStream dst,
                    SftpProgressMonitor monitor, int mode, long skip) throws SftpException{
    //System.err.println("_get: "+src+", "+dst);

    byte[] srcb=Util.str2byte(src, fEncoding);
    try{
      sendOPENR(srcb);

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_STATUS && type!=SSH_FXP_HANDLE){
	throw new SftpException(SSH_FX_FAILURE, "");
      }

      if(type==SSH_FXP_STATUS){
        int i=buf.getInt();
        throwStatusError(buf, i);
      }

      byte[] handle=buf.getString();         // filename

      long offset=0;
      if(mode==RESUME){
	offset+=skip;
      }

      int request_max=1;
      rq.init();
      long request_offset=offset;

      int request_len = buf.buffer.length-13;
      if(server_version==0){ request_len=1024; }

      loop:
      while(true){

        while(rq.count() < request_max){
          sendREAD(handle, request_offset, request_len, rq);
          request_offset += request_len;
        }

        header=header(buf, header);
        length=header.length;
        type=header.type;

        RequestQueue.Request rr = rq.get(header.rid);

        if(type==SSH_FXP_STATUS){
          fill(buf, length);
          int i=buf.getInt();    
          if(i==SSH_FX_EOF){
            break loop;
          }
          throwStatusError(buf, i);
        }

        if(type!=SSH_FXP_DATA){ 
	  break loop;
        }

        buf.rewind();
        fill(buf.buffer, 0, 4); length-=4;
        int length_of_data = buf.getInt();   // length of data 

        /**
         Since sftp protocol version 6, "end-of-file" has been defined,

           byte   SSH_FXP_DATA
           uint32 request-id
           string data
           bool   end-of-file [optional]

         but some sftpd server will send such a field in the sftp protocol 3 ;-(
         */
        int optional_data = length - length_of_data;

        int foo = length_of_data;
        while(foo>0){
          int bar=foo;
          if(bar>buf.buffer.length){
            bar=buf.buffer.length;
          }
          int data_len = io_in.read(buf.buffer, 0, bar);
          if(data_len<0){
            break loop;
	  }
          
          dst.write(buf.buffer, 0, data_len);

          offset+=data_len;
          foo-=data_len;

          if(monitor!=null){
            if(!monitor.count(data_len)){
              skip(foo); 
              if(optional_data>0){
                skip(optional_data);
              }
              break loop;
            }
          }

        }
	//System.err.println("length: "+length);  // length should be 0

        if(optional_data>0){
          skip(optional_data);
        }

        if(length_of_data<rr.length){  //
          rq.cancel(header, buf);
          sendREAD(handle, rr.offset+length_of_data, (int)(rr.length-length_of_data), rq);
          request_offset=rr.offset+rr.length;
        }

        if(request_max < rq.size()){
          request_max++;
        }
      }
      dst.flush();

      if(monitor!=null)monitor.end();

      rq.cancel(header, buf);

      _sendCLOSE(handle, header);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }


  private class RequestQueue {
    class Request {
      int id;
      long offset;
      long length;
    }

    Request[] rrq=null;
    int head, count;
    RequestQueue(int size){
      rrq = new Request[size];
      for(int i=0; i<rrq.length; i++){
        rrq[i]=new Request();
      }
      init();
    }

    void init(){
      head=count=0;
    }

    void add(int id, long offset, int length){
      if(count == 0) head = 0;
      int tail = head + count;
      if(tail>=rrq.length) tail -= rrq.length;
      rrq[tail].id=id;
      rrq[tail].offset=offset;
      rrq[tail].length=length;
      count++;
    }

    Request get(int id){
      count -= 1;
      int i=head;
      head++; 
      if(head==rrq.length) head=0;
      if(rrq[i].id!=id){
        System.err.println("The request is not in order.");
      }
      rrq[i].id=0;
      return rrq[i];
    }

    int count() {
      return count;
    }

    int size() {
      return rrq.length;
    } 

    void cancel(Header header, Buffer buf) throws IOException {
      int _count = count;
      for(int i=0; i<_count; i++){
        header=header(buf, header);
        int length=header.length;
        get(header.rid);
        skip(length); 
      }
    }
  }

  public InputStream get(String src) throws SftpException{
    return get(src, null, 0L);
  }
  public InputStream get(String src, SftpProgressMonitor monitor) throws SftpException{
    return get(src, monitor, 0L);
  }

  /**
   * @deprecated  This method will be deleted in the future.
   */
  public InputStream get(String src, int mode) throws SftpException{
    return get(src, null, 0L);
  }
  /**
   * @deprecated  This method will be deleted in the future.
   */
  public InputStream get(String src, final SftpProgressMonitor monitor, final int mode) throws SftpException{
    return get(src, monitor, 0L);
  }
  public InputStream get(String src, final SftpProgressMonitor monitor, final long skip) throws SftpException{

    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      src=remoteAbsolutePath(src);
      src=isUnique(src);

      byte[] srcb=Util.str2byte(src, fEncoding);

      SftpATTRS attr=_stat(srcb);
      if(monitor!=null){
        monitor.init(SftpProgressMonitor.GET, src, "??", attr.getSize());
      }

      sendOPENR(srcb);

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_STATUS && type!=SSH_FXP_HANDLE){
	throw new SftpException(SSH_FX_FAILURE, "");
      }
      if(type==SSH_FXP_STATUS){
        int i=buf.getInt();
        throwStatusError(buf, i);
      }

      final byte[] handle=buf.getString();         // handle

      java.io.InputStream in=new java.io.InputStream(){
           long offset=skip;
           boolean closed=false;
           int rest_length=0;
           byte[] _data=new byte[1];
           byte[] rest_byte=new byte[1024];
           Header header=new Header();

           public int read() throws java.io.IOException{
             if(closed)return -1;
             int i=read(_data, 0, 1);
             if (i==-1) { return -1; }
             else {
               return _data[0]&0xff;
             }
           }
           public int read(byte[] d) throws java.io.IOException{
             if(closed)return -1;
             return read(d, 0, d.length);
           }
           public int read(byte[] d, int s, int len) throws java.io.IOException{
             if(closed)return -1;
             if(d==null){throw new NullPointerException();}
             if(s<0 || len <0 || s+len>d.length){
               throw new IndexOutOfBoundsException();
             } 
             if(len==0){ return 0; }

             if(rest_length>0){
               int foo=rest_length;
               if(foo>len) foo=len;
               System.arraycopy(rest_byte, 0, d, s, foo);
               if(foo!=rest_length){
                 System.arraycopy(rest_byte, foo, 
                                  rest_byte, 0, rest_length-foo);
               }

               if(monitor!=null){
                 if(!monitor.count(foo)){
                   close();
                   return -1;
                 }
               }

               rest_length-=foo;
               return foo;
             }

             if(buf.buffer.length-13<len){
               len=buf.buffer.length-13;
             }
             if(server_version==0 && len>1024){
               len=1024; 
             }

             try{sendREAD(handle, offset, len);}
             catch(Exception e){ throw new IOException("error"); }

             header=header(buf, header);
             rest_length=header.length;
             int type=header.type;
             int id=header.rid;

             if(type!=SSH_FXP_STATUS && type!=SSH_FXP_DATA){ 
               throw new IOException("error");
             }
             if(type==SSH_FXP_STATUS){
               fill(buf, rest_length);
               int i=buf.getInt();    
               rest_length=0;
               if(i==SSH_FX_EOF){
                 close();
                 return -1;
               }
               //throwStatusError(buf, i);
               throw new IOException("error");
             }
             buf.rewind();
             fill(buf.buffer, 0, 4);
             int length_of_data = buf.getInt(); rest_length-=4;

             /**
              Since sftp protocol version 6, "end-of-file" has been defined,
     
                byte   SSH_FXP_DATA
                uint32 request-id
                string data
                bool   end-of-file [optional]
     
              but some sftpd server will send such a field in the sftp protocol 3 ;-(
              */
             int optional_data = rest_length - length_of_data;

             offset += length_of_data;
             int foo = length_of_data;
             if(foo>0){
               int bar=foo;
               if(bar>len){
                 bar=len;
               }
               int i=io_in.read(d, s, bar);
               if(i<0){
                 return -1;
               }
               foo-=i;
               rest_length=foo;

               if(foo>0){
                 if(rest_byte.length<foo){
                   rest_byte=new byte[foo];
                 }
                 int _s=0;
                 int _len=foo;
                 int j;
                 while(_len>0){
                   j=io_in.read(rest_byte, _s, _len);
                   if(j<=0)break;
                   _s+=j;
                   _len-=j;
                 }
               }

               if(optional_data>0){
                 io_in.skip(optional_data);
               }

               if(monitor!=null){
                 if(!monitor.count(i)){
                   close();
                   return -1;
                 }
               }

               return i;
             }
             return 0; // ??
           }
           public void close() throws IOException{
             if(closed)return;
             closed=true;
             if(monitor!=null)monitor.end();
             try{_sendCLOSE(handle, header);}
             catch(Exception e){throw new IOException("error");}
           }
         };
       return in;
     }
     catch(Exception e){
       if(e instanceof SftpException) throw (SftpException)e;
       if(e instanceof Throwable)
         throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
       throw new SftpException(SSH_FX_FAILURE, "");
     }
   }

   public java.util.Vector ls(String path) throws SftpException{
     //System.out.println("ls: "+path);
     try{
       ((MyPipedInputStream)io_in).updateReadSide();

       path=remoteAbsolutePath(path);
       byte[] pattern=null;
       java.util.Vector v=new java.util.Vector();

       int foo=path.lastIndexOf('/');
       String dir=path.substring(0, ((foo==0)?1:foo));
       String _pattern=path.substring(foo+1);
       dir=Util.unquote(dir);

       // If pattern has included '*' or '?', we need to convert
       // to UTF-8 string before globbing.
       byte[][] _pattern_utf8=new byte[1][];
       boolean pattern_has_wildcard=isPattern(_pattern, _pattern_utf8);

       if(pattern_has_wildcard){
         pattern=_pattern_utf8[0];
       }
       else{
         String upath=Util.unquote(path);
         //SftpATTRS attr=_lstat(upath);
         SftpATTRS attr=_stat(upath);
         if(attr.isDir()){
           pattern=null;
           dir=upath;
         }
         else{
           /*
             // If we can generage longname by ourself,
             // we don't have to use openDIR.
           String filename=Util.unquote(_pattern);
           String longname=...
           v.addElement(new LsEntry(filename, longname, attr));
           return v;
           */

           if(fEncoding_is_utf8){
             pattern=_pattern_utf8[0];
             pattern=Util.unquote(pattern);
           }
           else{
             _pattern=Util.unquote(_pattern);
             pattern=Util.str2byte(_pattern, fEncoding);
           }

         }
       }

       sendOPENDIR(Util.str2byte(dir, fEncoding));

       Header header=new Header();
       header=header(buf, header);
       int length=header.length;
       int type=header.type;

       fill(buf, length);

       if(type!=SSH_FXP_STATUS && type!=SSH_FXP_HANDLE){
         throw new SftpException(SSH_FX_FAILURE, "");
       }
       if(type==SSH_FXP_STATUS){
         int i=buf.getInt();
         throwStatusError(buf, i);
       }

       byte[] handle=buf.getString();         // handle

       while(true){
         sendREADDIR(handle);

         header=header(buf, header);
         length=header.length;
         type=header.type;
         if(type!=SSH_FXP_STATUS && type!=SSH_FXP_NAME){
           throw new SftpException(SSH_FX_FAILURE, "");
         }
         if(type==SSH_FXP_STATUS){ 
           fill(buf, length);
           int i=buf.getInt();
           if(i==SSH_FX_EOF)
             break;
           throwStatusError(buf, i);
         }

         buf.rewind();
         fill(buf.buffer, 0, 4); length-=4;
         int count=buf.getInt();

         byte[] str;
         int flags;

         buf.reset();
         while(count>0){
           if(length>0){
             buf.shift();
             int j=(buf.buffer.length>(buf.index+length)) ? 
               length : 
               (buf.buffer.length-buf.index);
             int i=fill(buf.buffer, buf.index, j);
             buf.index+=i;
             length-=i;
           }
           byte[] filename=buf.getString();
           byte[] longname=null;
           if(server_version<=3){
             longname=buf.getString();
           }
           SftpATTRS attrs=SftpATTRS.getATTR(buf);

           boolean find=false;
           String f=null;
           if(pattern==null){
             find=true;
           }
           else if(!pattern_has_wildcard){
             find=Util.array_equals(pattern, filename);
           }
           else{
             byte[] _filename=filename;
             if(!fEncoding_is_utf8){
               f=Util.byte2str(_filename, fEncoding);
               _filename=Util.str2byte(f, UTF8);
             }
             find=Util.glob(pattern, _filename);
           }

           if(find){
             if(f==null){
               f=Util.byte2str(filename, fEncoding);
             }
             String l=null;
             if(longname==null){
               // TODO: we need to generate long name from attrs
               //       for the sftp protocol 4(and later).
               l=attrs.toString()+" "+f;
             }
             else{
               l=Util.byte2str(longname, fEncoding);
             }
             v.addElement(new LsEntry(f, l, attrs));
           }

           count--; 
         }
       }
       _sendCLOSE(handle, header);

       /*
       if(v.size()==1 && pattern_has_wildcard){
         LsEntry le=(LsEntry)v.elementAt(0);
         if(le.getAttrs().isDir()){
           String f=le.getFilename();
           if(isPattern(f)){
             f=Util.quote(f);
           }
           if(!dir.endsWith("/")){
             dir+="/";
           }
           v=null;
           return ls(dir+f);
         }
       }
       */

       return v;
     }
     catch(Exception e){
       if(e instanceof SftpException) throw (SftpException)e;
       if(e instanceof Throwable)
         throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
       throw new SftpException(SSH_FX_FAILURE, "");
     }
   }
   public String readlink(String path) throws SftpException{
     try{
       if(server_version<3){
         throw new SftpException(SSH_FX_OP_UNSUPPORTED, 
                                 "The remote sshd is too old to support symlink operation.");
       }

       ((MyPipedInputStream)io_in).updateReadSide();

       path=remoteAbsolutePath(path);

       path=isUnique(path);

       sendREADLINK(Util.str2byte(path, fEncoding));

       Header header=new Header();
       header=header(buf, header);
       int length=header.length;
       int type=header.type;

       fill(buf, length);

       if(type!=SSH_FXP_STATUS && type!=SSH_FXP_NAME){
         throw new SftpException(SSH_FX_FAILURE, "");
       }
       if(type==SSH_FXP_NAME){
         int count=buf.getInt();       // count
         byte[] filename=null;
         for(int i=0; i<count; i++){
           filename=buf.getString();
           if(server_version<=3){
             byte[] longname=buf.getString();
           }
           SftpATTRS.getATTR(buf);
         }
         return Util.byte2str(filename, fEncoding);
       }

       int i=buf.getInt();
       throwStatusError(buf, i);
     }
     catch(Exception e){
       if(e instanceof SftpException) throw (SftpException)e;
       if(e instanceof Throwable)
         throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
       throw new SftpException(SSH_FX_FAILURE, "");
     }
     return null;
   }
   public void symlink(String oldpath, String newpath) throws SftpException{
     if(server_version<3){
       throw new SftpException(SSH_FX_OP_UNSUPPORTED, 
                               "The remote sshd is too old to support symlink operation.");
     }

     try{
       ((MyPipedInputStream)io_in).updateReadSide();

       oldpath=remoteAbsolutePath(oldpath);
       newpath=remoteAbsolutePath(newpath);

       oldpath=isUnique(oldpath);

       if(isPattern(newpath)){
         throw new SftpException(SSH_FX_FAILURE, newpath);
       }
       newpath=Util.unquote(newpath);

       sendSYMLINK(Util.str2byte(oldpath, fEncoding),
                   Util.str2byte(newpath, fEncoding));

       Header header=new Header();
       header=header(buf, header);
       int length=header.length;
       int type=header.type;

       fill(buf, length);

       if(type!=SSH_FXP_STATUS){
         throw new SftpException(SSH_FX_FAILURE, "");
       }

       int i=buf.getInt();
       if(i==SSH_FX_OK) return;
       throwStatusError(buf, i);
     }
     catch(Exception e){
       if(e instanceof SftpException) throw (SftpException)e;
       if(e instanceof Throwable)
         throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
       throw new SftpException(SSH_FX_FAILURE, "");
     }
   }

   public void rename(String oldpath, String newpath) throws SftpException{
     if(server_version<2){
       throw new SftpException(SSH_FX_OP_UNSUPPORTED, 
                               "The remote sshd is too old to support rename operation.");
     }

     try{
       ((MyPipedInputStream)io_in).updateReadSide();

       oldpath=remoteAbsolutePath(oldpath);
       newpath=remoteAbsolutePath(newpath);

       oldpath=isUnique(oldpath);

       Vector v=glob_remote(newpath);
       int vsize=v.size();
       if(vsize>=2){
         throw new SftpException(SSH_FX_FAILURE, v.toString());
       }
       if(vsize==1){
         newpath=(String)(v.elementAt(0));
       }
       else{  // vsize==0
         if(isPattern(newpath))
           throw new SftpException(SSH_FX_FAILURE, newpath);
         newpath=Util.unquote(newpath);
       }

       sendRENAME(Util.str2byte(oldpath, fEncoding),
                  Util.str2byte(newpath, fEncoding));

       Header header=new Header();
       header=header(buf, header);
       int length=header.length;
       int type=header.type;

       fill(buf, length);

       if(type!=SSH_FXP_STATUS){
         throw new SftpException(SSH_FX_FAILURE, "");
       }

       int i=buf.getInt();
       if(i==SSH_FX_OK) return;
       throwStatusError(buf, i);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }
  public void rm(String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();

      Header header=new Header();

      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));
        sendREMOVE(Util.str2byte(path, fEncoding));

        header=header(buf, header);
        int length=header.length;
        int type=header.type;

        fill(buf, length);

        if(type!=SSH_FXP_STATUS){
	  throw new SftpException(SSH_FX_FAILURE, "");
        }
        int i=buf.getInt();
	if(i!=SSH_FX_OK){
	  throwStatusError(buf, i);
	}
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  private boolean isRemoteDir(String path){
    try{
      sendSTAT(Util.str2byte(path, fEncoding));

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_ATTRS){
        return false; 
      }
      SftpATTRS attr=SftpATTRS.getATTR(buf);
      return attr.isDir();
    }
    catch(Exception e){}
    return false;
  }

  public void chgrp(int gid, String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();
      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));

        SftpATTRS attr=_stat(path);

	attr.setFLAGS(0);
	attr.setUIDGID(attr.uid, gid); 
	_setStat(path, attr);
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void chown(int uid, String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();
      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));

        SftpATTRS attr=_stat(path);

	attr.setFLAGS(0);
	attr.setUIDGID(uid, attr.gid); 
	_setStat(path, attr);
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void chmod(int permissions, String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();
      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));

	SftpATTRS attr=_stat(path);

	attr.setFLAGS(0);
	attr.setPERMISSIONS(permissions); 
	_setStat(path, attr);
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void setMtime(String path, int mtime) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();
      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));

        SftpATTRS attr=_stat(path);

	attr.setFLAGS(0);
	attr.setACMODTIME(attr.getATime(), mtime);
	_setStat(path, attr);
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void rmdir(String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();

      Header header=new Header();

      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));
	sendRMDIR(Util.str2byte(path, fEncoding));

        header=header(buf, header);
        int length=header.length;
        int type=header.type;

        fill(buf, length);

	if(type!=SSH_FXP_STATUS){
	  throw new SftpException(SSH_FX_FAILURE, "");
	}

	int i=buf.getInt();
	if(i!=SSH_FX_OK){
	  throwStatusError(buf, i);
	}
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public void mkdir(String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      sendMKDIR(Util.str2byte(path, fEncoding), null);

      Header header=new Header();      
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_STATUS){
	throw new SftpException(SSH_FX_FAILURE, "");
      }

      int i=buf.getInt();
      if(i==SSH_FX_OK) return;
      throwStatusError(buf, i);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public SftpATTRS stat(String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);
      path=isUnique(path);

      return _stat(path);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
    //return null;
  }

  private SftpATTRS _stat(byte[] path) throws SftpException{
    try{

      sendSTAT(path);

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_ATTRS){
	if(type==SSH_FXP_STATUS){
	  int i=buf.getInt();
	  throwStatusError(buf, i);
	}
	throw new SftpException(SSH_FX_FAILURE, "");
      }
      SftpATTRS attr=SftpATTRS.getATTR(buf);
      return attr;
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
    //return null;
  }

  private SftpATTRS _stat(String path) throws SftpException{
    return _stat(Util.str2byte(path, fEncoding));
  }

  public SftpATTRS lstat(String path) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);
      path=isUnique(path);

      return _lstat(path);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  private SftpATTRS _lstat(String path) throws SftpException{
    try{
      sendLSTAT(Util.str2byte(path, fEncoding));

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_ATTRS){
	if(type==SSH_FXP_STATUS){
	  int i=buf.getInt();
	  throwStatusError(buf, i);
	}
	throw new SftpException(SSH_FX_FAILURE, "");
      }
      SftpATTRS attr=SftpATTRS.getATTR(buf);
      return attr;
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  private byte[] _realpath(String path) throws SftpException, IOException, Exception{
    sendREALPATH(Util.str2byte(path, fEncoding));

    Header header=new Header();
    header=header(buf, header);
    int length=header.length;
    int type=header.type;

    fill(buf, length);

    if(type!=SSH_FXP_STATUS && type!=SSH_FXP_NAME){
      throw new SftpException(SSH_FX_FAILURE, "");
    }
    int i;
    if(type==SSH_FXP_STATUS){
      i=buf.getInt();
      throwStatusError(buf, i);
    }
    i=buf.getInt();   // count

    byte[] str=null;
    while(i-->0){
      str=buf.getString();  // absolute path;
      if(server_version<=3){
        byte[] lname=buf.getString();  // long filename
      }
      SftpATTRS attr=SftpATTRS.getATTR(buf);  // dummy attribute
    }
    return str;
  }

  public void setStat(String path, SftpATTRS attr) throws SftpException{
    try{
      ((MyPipedInputStream)io_in).updateReadSide();

      path=remoteAbsolutePath(path);

      Vector v=glob_remote(path);
      int vsize=v.size();
      for(int j=0; j<vsize; j++){
	path=(String)(v.elementAt(j));
	_setStat(path, attr);
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }
  private void _setStat(String path, SftpATTRS attr) throws SftpException{
    try{
      sendSETSTAT(Util.str2byte(path, fEncoding), attr);

      Header header=new Header();
      header=header(buf, header);
      int length=header.length;
      int type=header.type;

      fill(buf, length);

      if(type!=SSH_FXP_STATUS){
	throw new SftpException(SSH_FX_FAILURE, "");
      }
      int i=buf.getInt();
      if(i!=SSH_FX_OK){
	throwStatusError(buf, i);
      }
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public String pwd() throws SftpException{ return getCwd(); }
  public String lpwd(){ return lcwd; }
  public String version(){ return version; }
  public String getHome() throws SftpException {
    if(home==null){
      try{
        ((MyPipedInputStream)io_in).updateReadSide();

        byte[] _home=_realpath("");
        home=Util.byte2str(_home, fEncoding);
      }
      catch(Exception e){
        if(e instanceof SftpException) throw (SftpException)e;
        if(e instanceof Throwable)
          throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
        throw new SftpException(SSH_FX_FAILURE, "");
      }
    }
    return home; 
  }

  private String getCwd() throws SftpException{
    if(cwd==null)
      cwd=getHome();
    return cwd;
  }

  private void setCwd(String cwd){
    this.cwd=cwd;
  }

  private void read(byte[] buf, int s, int l) throws IOException, SftpException{
    int i=0;
    while(l>0){
      i=io_in.read(buf, s, l);
      if(i<=0){
        throw new SftpException(SSH_FX_FAILURE, "");
      }
      s+=i;
      l-=i;
    }
  }

  private boolean checkStatus(int[] ackid, Header header) throws IOException, SftpException{
    header=header(buf, header);
    int length=header.length;
    int type=header.type;
    if(ackid!=null)
      ackid[0]=header.rid;

    fill(buf, length);

    if(type!=SSH_FXP_STATUS){ 
      throw new SftpException(SSH_FX_FAILURE, "");
    }
    int i=buf.getInt();
    if(i!=SSH_FX_OK){
      throwStatusError(buf, i);
    }
    return true;
  }
  private boolean _sendCLOSE(byte[] handle, Header header) throws Exception{
    sendCLOSE(handle);
    return checkStatus(null, header);
  }

  private void sendINIT() throws Exception{
    packet.reset();
    putHEAD(SSH_FXP_INIT, 5);
    buf.putInt(3);                // version 3
    getSession().write(packet, this, 5+4);
  }

  private void sendREALPATH(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_REALPATH, path);
  }
  private void sendSTAT(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_STAT, path);
  }
  private void sendLSTAT(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_LSTAT, path);
  }
  private void sendFSTAT(byte[] handle) throws Exception{
    sendPacketPath(SSH_FXP_FSTAT, handle);
  }
  private void sendSETSTAT(byte[] path, SftpATTRS attr) throws Exception{
    packet.reset();
    putHEAD(SSH_FXP_SETSTAT, 9+path.length+attr.length());
    buf.putInt(seq++);
    buf.putString(path);             // path
    attr.dump(buf);
    getSession().write(packet, this, 9+path.length+attr.length()+4);
  }
  private void sendREMOVE(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_REMOVE, path);
  }
  private void sendMKDIR(byte[] path, SftpATTRS attr) throws Exception{
    packet.reset();
    putHEAD(SSH_FXP_MKDIR, 9+path.length+(attr!=null?attr.length():4));
    buf.putInt(seq++);
    buf.putString(path);             // path
    if(attr!=null) attr.dump(buf);
    else buf.putInt(0);
    getSession().write(packet, this, 9+path.length+(attr!=null?attr.length():4)+4);
  }
  private void sendRMDIR(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_RMDIR, path);
  }
  private void sendSYMLINK(byte[] p1, byte[] p2) throws Exception{
    sendPacketPath(SSH_FXP_SYMLINK, p1, p2);
  }
  private void sendREADLINK(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_READLINK, path);
  }
  private void sendOPENDIR(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_OPENDIR, path);
  }
  private void sendREADDIR(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_READDIR, path);
  }
  private void sendRENAME(byte[] p1, byte[] p2) throws Exception{
    sendPacketPath(SSH_FXP_RENAME, p1, p2);
  }
  private void sendCLOSE(byte[] path) throws Exception{
    sendPacketPath(SSH_FXP_CLOSE, path);
  }
  private void sendOPENR(byte[] path) throws Exception{
    sendOPEN(path, SSH_FXF_READ);
  }
  private void sendOPENW(byte[] path) throws Exception{
    sendOPEN(path, SSH_FXF_WRITE|SSH_FXF_CREAT|SSH_FXF_TRUNC);
  }
  private void sendOPENA(byte[] path) throws Exception{
    sendOPEN(path, SSH_FXF_WRITE|/*SSH_FXF_APPEND|*/SSH_FXF_CREAT);
  }
  private void sendOPEN(byte[] path, int mode) throws Exception{
    packet.reset();
    putHEAD(SSH_FXP_OPEN, 17+path.length);
    buf.putInt(seq++);
    buf.putString(path);
    buf.putInt(mode);
    buf.putInt(0);           // attrs
    getSession().write(packet, this, 17+path.length+4);
  }
  private void sendPacketPath(byte fxp, byte[] path) throws Exception{
    packet.reset();
    putHEAD(fxp, 9+path.length);
    buf.putInt(seq++);
    buf.putString(path);             // path
    getSession().write(packet, this, 9+path.length+4);
  }
  private void sendPacketPath(byte fxp, byte[] p1, byte[] p2) throws Exception{
    packet.reset();
    putHEAD(fxp, 13+p1.length+p2.length);
    buf.putInt(seq++);
    buf.putString(p1);
    buf.putString(p2);
    getSession().write(packet, this, 13+p1.length+p2.length+4);
  }

  private int sendWRITE(byte[] handle, long offset, 
                        byte[] data, int start, int length) throws Exception{
    int _length=length;
    opacket.reset();
    if(obuf.buffer.length<obuf.index+13+21+handle.length+length+Session.buffer_margin){
      _length=obuf.buffer.length-(obuf.index+13+21+handle.length+Session.buffer_margin);
      // System.err.println("_length="+_length+" length="+length);
    }

    putHEAD(obuf, SSH_FXP_WRITE, 21+handle.length+_length);       // 14
    obuf.putInt(seq++);                                      //  4
    obuf.putString(handle);                                  //  4+handle.length
    obuf.putLong(offset);                                    //  8
    if(obuf.buffer!=data){
      obuf.putString(data, start, _length);                    //  4+_length
    }
    else{
      obuf.putInt(_length);
      obuf.skip(_length);
    }
    getSession().write(opacket, this, 21+handle.length+_length+4);
    return _length;
  }
  private void sendREAD(byte[] handle, long offset, int length) throws Exception{
    sendREAD(handle, offset, length, null);
  }
  private void sendREAD(byte[] handle, long offset, int length,
                        RequestQueue rrq) throws Exception{
    packet.reset();
    putHEAD(SSH_FXP_READ, 21+handle.length);
    buf.putInt(seq++);
    buf.putString(handle);
    buf.putLong(offset);
    buf.putInt(length);
    getSession().write(packet, this, 21+handle.length+4);
    if(rrq!=null){
      rrq.add(seq-1, offset, length);
    }
  }

  private void putHEAD(Buffer buf, byte type, int length) throws Exception{
    buf.putByte((byte)Session.SSH_MSG_CHANNEL_DATA);
    buf.putInt(recipient);
    buf.putInt(length+4);
    buf.putInt(length);
    buf.putByte(type);
  }

  private void putHEAD(byte type, int length) throws Exception{
    putHEAD(buf, type, length);
  }

  private Vector glob_remote(String _path) throws Exception{
    Vector v=new Vector();
    int i=0;

    int foo=_path.lastIndexOf('/');
    if(foo<0){  // it is not absolute path.
      v.addElement(Util.unquote(_path)); 
      return v;
    }

    String dir=_path.substring(0, ((foo==0)?1:foo));
    String _pattern=_path.substring(foo+1);

    dir=Util.unquote(dir);

    byte[] pattern=null;
    byte[][] _pattern_utf8=new byte[1][];
    boolean pattern_has_wildcard=isPattern(_pattern, _pattern_utf8);

    if(!pattern_has_wildcard){
      if(!dir.equals("/"))
        dir+="/";
      v.addElement(dir+Util.unquote(_pattern));
      return v;
    }

    pattern=_pattern_utf8[0];

    sendOPENDIR(Util.str2byte(dir, fEncoding));

    Header header=new Header();
    header=header(buf, header);
    int length=header.length;
    int type=header.type;

    fill(buf, length);

    if(type!=SSH_FXP_STATUS && type!=SSH_FXP_HANDLE){
      throw new SftpException(SSH_FX_FAILURE, "");
    }
    if(type==SSH_FXP_STATUS){
      i=buf.getInt();
      throwStatusError(buf, i);
    }

    byte[] handle=buf.getString();         // filename
    String pdir=null;                      // parent directory

    while(true){
      sendREADDIR(handle);
      header=header(buf, header);
      length=header.length;
      type=header.type;

      if(type!=SSH_FXP_STATUS && type!=SSH_FXP_NAME){
	throw new SftpException(SSH_FX_FAILURE, "");
      }
      if(type==SSH_FXP_STATUS){ 
        fill(buf, length);
	break;
      }

      buf.rewind();
      fill(buf.buffer, 0, 4); length-=4;
      int count=buf.getInt();

      byte[] str;
      int flags;

      buf.reset();
      while(count>0){
	if(length>0){
	  buf.shift();
          int j=(buf.buffer.length>(buf.index+length)) ? length : (buf.buffer.length-buf.index);
	  i=io_in.read(buf.buffer, buf.index, j);
	  if(i<=0)break;
	  buf.index+=i;
	  length-=i;
	}

	byte[] filename=buf.getString();
	//System.err.println("filename: "+new String(filename));
        if(server_version<=3){
          str=buf.getString();  // longname
        }
	SftpATTRS attrs=SftpATTRS.getATTR(buf);

        byte[] _filename=filename;
        String f=null;
        boolean found=false;

        if(!fEncoding_is_utf8){
          f=Util.byte2str(filename, fEncoding);
          _filename=Util.str2byte(f, UTF8);
        }
        found=Util.glob(pattern, _filename);

	if(found){
          if(f==null){
            f=Util.byte2str(filename, fEncoding);
          }
          if(pdir==null){
            pdir=dir;
            if(!pdir.endsWith("/")){
              pdir+="/";
            }
          }
	  v.addElement(pdir+f);
	}
	count--; 
      }
    }
    if(_sendCLOSE(handle, header)) 
      return v;
    return null;
  }

  private boolean isPattern(byte[] path){
    int length=path.length;
    int i=0;
    while(i<length){
      if(path[i]=='*' || path[i]=='?')
        return true;
      if(path[i]=='\\' && (i+1)<length)
        i++;
      i++;
    }
    return false;
  }

  private Vector glob_local(String _path) throws Exception{
//System.err.println("glob_local: "+_path);
    Vector v=new Vector();
    byte[] path=Util.str2byte(_path, UTF8);
    int i=path.length-1;
    while(i>=0){
      if(path[i]!='*' && path[i]!='?'){
        i--;
        continue;
      }
      if(!fs_is_bs &&
         i>0 && path[i-1]=='\\'){
        i--;
        if(i>0 && path[i-1]=='\\'){
          i--;
          i--;
          continue;
        }
      }
      break;
    }

    if(i<0){ v.addElement(fs_is_bs ? _path : Util.unquote(_path)); return v;}

    while(i>=0){
      if(path[i]==file_separatorc ||
         (fs_is_bs && path[i]=='/')){ // On Windows, '/' is also the separator.
        break;
      }
      i--;
    }

    if(i<0){ v.addElement(fs_is_bs ? _path : Util.unquote(_path)); return v;}

    byte[] dir;
    if(i==0){dir=new byte[]{(byte)file_separatorc};}
    else{ 
      dir=new byte[i];
      System.arraycopy(path, 0, dir, 0, i);
    }

    byte[] pattern=new byte[path.length-i-1];
    System.arraycopy(path, i+1, pattern, 0, pattern.length);

//System.err.println("dir: "+new String(dir)+" pattern: "+new String(pattern));
    try{
      String[] children=(new File(Util.byte2str(dir, UTF8))).list();
      String pdir=Util.byte2str(dir)+file_separator;
      for(int j=0; j<children.length; j++){
//System.err.println("children: "+children[j]);
	if(Util.glob(pattern, Util.str2byte(children[j], UTF8))){
	  v.addElement(pdir+children[j]);
	}
      }
    }
    catch(Exception e){
    }
    return v;
  }

  private void throwStatusError(Buffer buf, int i) throws SftpException{
    if(server_version>=3 &&   // WindRiver's sftp will send invalid 
       buf.getLength()>=4){   // SSH_FXP_STATUS packet.
      byte[] str=buf.getString();
      //byte[] tag=buf.getString();
      throw new SftpException(i, Util.byte2str(str, UTF8));
    }
    else{
      throw new SftpException(i, "Failure");
    }
  }

  private static boolean isLocalAbsolutePath(String path){
    return (new File(path)).isAbsolute();
  }

  public void disconnect(){
    super.disconnect();
  }

  private boolean isPattern(String path, byte[][] utf8){
    byte[] _path=Util.str2byte(path, UTF8);
    if(utf8!=null)
      utf8[0]=_path;
    return isPattern(_path);
  }

  private boolean isPattern(String path){
    return isPattern(path, null);
  }

  private void fill(Buffer buf, int len)  throws IOException{
    buf.reset();
    fill(buf.buffer, 0, len);
    buf.skip(len);
  }

  private int fill(byte[] buf, int s, int len) throws IOException{
    int i=0;
    int foo=s;
    while(len>0){
      i=io_in.read(buf, s, len);
      if(i<=0){
        throw new IOException("inputstream is closed");
        //return (s-foo)==0 ? i : s-foo;
      }
      s+=i;
      len-=i;
    }
    return s-foo;
  }
  private void skip(long foo) throws IOException{
    while(foo>0){
      long bar=io_in.skip(foo);
      if(bar<=0) 
        break;
      foo-=bar;
    }
  }

  class Header{
    int length;
    int type;
    int rid;
  }
  private Header header(Buffer buf, Header header) throws IOException{
    buf.rewind();
    int i=fill(buf.buffer, 0, 9);
    header.length=buf.getInt()-5;
    header.type=buf.getByte()&0xff;
    header.rid=buf.getInt();  
    return header;
  }

  private String remoteAbsolutePath(String path) throws SftpException{
    if(path.charAt(0)=='/') return path;
    String cwd=getCwd();
//    if(cwd.equals(getHome())) return path;
    if(cwd.endsWith("/")) return cwd+path;
    return cwd+"/"+path;
  }

  private String localAbsolutePath(String path){
    if(isLocalAbsolutePath(path)) return path;
    if(lcwd.endsWith(file_separator)) return lcwd+path;
    return lcwd+file_separator+path;
  }

  /**
   * This method will check if the given string can be expanded to the
   * unique string.  If it can be expanded to mutiple files, SftpException
   * will be thrown.
   * @return the returned string is unquoted.
   */
  private String isUnique(String path) throws SftpException, Exception{
    Vector v=glob_remote(path);
    if(v.size()!=1){
      throw new SftpException(SSH_FX_FAILURE, path+" is not unique: "+v.toString());
    }
    return (String)(v.elementAt(0));
  }

  public int getServerVersion() throws SftpException{
    if(!isConnected()){
      throw new SftpException(SSH_FX_FAILURE, "The channel is not connected.");
    }
    return server_version;
  }

  public void setFilenameEncoding(String encoding) throws SftpException{
    int sversion=getServerVersion();
    if(3 <= sversion && sversion <= 5 &&
       !encoding.equals(UTF8)){
      throw new SftpException(SSH_FX_FAILURE,
                              "The encoding can not be changed for this sftp server.");
    }
    if(encoding.equals(UTF8)){
      encoding=UTF8;
    }
    fEncoding=encoding;
    fEncoding_is_utf8=fEncoding.equals(UTF8);
  }

  public String getExtension(String key){
    if(extensions==null)
      return null;
    return (String)extensions.get(key);
  }

  public String realpath(String path) throws SftpException{
    try{
      byte[] _path=_realpath(remoteAbsolutePath(path));
      return Util.byte2str(_path, fEncoding);
    }
    catch(Exception e){
      if(e instanceof SftpException) throw (SftpException)e;
      if(e instanceof Throwable)
        throw new SftpException(SSH_FX_FAILURE, "", (Throwable)e);
      throw new SftpException(SSH_FX_FAILURE, "");
    }
  }

  public class LsEntry implements Comparable{
    private  String filename;
    private  String longname;
    private  SftpATTRS attrs;
    LsEntry(String filename, String longname, SftpATTRS attrs){
      setFilename(filename);
      setLongname(longname);
      setAttrs(attrs);
    }
    public String getFilename(){return filename;};
    void setFilename(String filename){this.filename = filename;};
    public String getLongname(){return longname;};
    void setLongname(String longname){this.longname = longname;};
    public SftpATTRS getAttrs(){return attrs;};
    void setAttrs(SftpATTRS attrs) {this.attrs = attrs;};
    public String toString(){ return longname; }
    public int compareTo(Object o) throws ClassCastException{
      if(o instanceof LsEntry){
        return filename.compareTo(((LsEntry)o).getFilename());
      }
      throw new ClassCastException("a decendent of LsEntry must be given.");
    }
  }
}
