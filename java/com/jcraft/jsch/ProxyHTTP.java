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
import java.net.*;

public class ProxyHTTP implements Proxy{
  private static int DEFAULTPORT=80;
  private String proxy_host;
  private int proxy_port;
  private InputStream in;
  private OutputStream out;
  private Socket socket;

  private String user;
  private String passwd;

  public ProxyHTTP(String proxy_host){
    int port=DEFAULTPORT;
    String host=proxy_host;
    if(proxy_host.indexOf(':')!=-1){
      try{
	host=proxy_host.substring(0, proxy_host.indexOf(':'));
	port=Integer.parseInt(proxy_host.substring(proxy_host.indexOf(':')+1));
      }
      catch(Exception e){
      }
    }
    this.proxy_host=host;
    this.proxy_port=port;
  }
  public ProxyHTTP(String proxy_host, int proxy_port){
    this.proxy_host=proxy_host;
    this.proxy_port=proxy_port;
  }
  public void setUserPasswd(String user, String passwd){
    this.user=user;
    this.passwd=passwd;
  }
  public void connect(SocketFactory socket_factory, String host, int port, int timeout) throws JSchException{
    try{
      if(socket_factory==null){
        socket=Util.createSocket(proxy_host, proxy_port, timeout);    
        in=socket.getInputStream();
        out=socket.getOutputStream();
      }
      else{
        socket=socket_factory.createSocket(proxy_host, proxy_port);
        in=socket_factory.getInputStream(socket);
        out=socket_factory.getOutputStream(socket);
      }
      if(timeout>0){
        socket.setSoTimeout(timeout);
      }
      socket.setTcpNoDelay(true);

      out.write(Util.str2byte("CONNECT "+host+":"+port+" HTTP/1.0\r\n"));

      if(user!=null && passwd!=null){
	byte[] code=Util.str2byte(user+":"+passwd);
	code=Util.toBase64(code, 0, code.length);
	out.write(Util.str2byte("Proxy-Authorization: Basic "));
	out.write(code);
	out.write(Util.str2byte("\r\n"));
      }

      out.write(Util.str2byte("\r\n"));
      out.flush();

      int foo=0;

      StringBuffer sb=new StringBuffer();
      while(foo>=0){
        foo=in.read(); if(foo!=13){sb.append((char)foo);  continue;}
        foo=in.read(); if(foo!=10){continue;}
        break;
      }
      if(foo<0){
        throw new IOException();
      }

      String response=sb.toString(); 
      String reason="Unknow reason";
      int code=-1;
      try{
        foo=response.indexOf(' ');
        int bar=response.indexOf(' ', foo+1);
        code=Integer.parseInt(response.substring(foo+1, bar));
        reason=response.substring(bar+1);
      }
      catch(Exception e){
      }
      if(code!=200){
        throw new IOException("proxy error: "+reason);
      }

      /*
      while(foo>=0){
        foo=in.read(); if(foo!=13) continue;
        foo=in.read(); if(foo!=10) continue;
        foo=in.read(); if(foo!=13) continue;      
        foo=in.read(); if(foo!=10) continue;
        break;
      }
      */

      int count=0;
      while(true){
        count=0;
        while(foo>=0){
          foo=in.read(); if(foo!=13){count++;  continue;}
          foo=in.read(); if(foo!=10){continue;}
          break;
        }
        if(foo<0){
          throw new IOException();
        }
        if(count==0)break;
      }
    }
    catch(RuntimeException e){
      throw e;
    }
    catch(Exception e){
      try{ if(socket!=null)socket.close(); }
      catch(Exception eee){
      }
      String message="ProxyHTTP: "+e.toString();
      if(e instanceof Throwable)
        throw new JSchException(message, (Throwable)e);
      throw new JSchException(message);
    }
  }
  public InputStream getInputStream(){ return in; }
  public OutputStream getOutputStream(){ return out; }
  public Socket getSocket(){ return socket; }
  public void close(){
    try{
      if(in!=null)in.close();
      if(out!=null)out.close();
      if(socket!=null)socket.close();
    }
    catch(Exception e){
    }
    in=null;
    out=null;
    socket=null;
  }
  public static int getDefaultPort(){
    return DEFAULTPORT;
  }
}
