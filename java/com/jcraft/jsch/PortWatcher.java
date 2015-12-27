/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2002-2015 ymnk, JCraft,Inc. All rights reserved.

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

import java.net.*;
import java.io.*;

class PortWatcher implements Runnable{
  private static java.util.Vector pool=new java.util.Vector();
  private static InetAddress anyLocalAddress=null;
  static{
    // 0.0.0.0
/*
    try{ anyLocalAddress=InetAddress.getByAddress(new byte[4]); }
    catch(UnknownHostException e){
    }
*/
    try{ anyLocalAddress=InetAddress.getByName("0.0.0.0"); }
    catch(UnknownHostException e){
    }
  }

  Session session;
  int lport;
  int rport;
  String host;
  InetAddress boundaddress;
  Runnable thread;
  ServerSocket ss;
  int connectTimeout=0;

  static String[] getPortForwarding(Session session){
    java.util.Vector foo=new java.util.Vector();
    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
	PortWatcher p=(PortWatcher)(pool.elementAt(i));
	if(p.session==session){
	  foo.addElement(p.lport+":"+p.host+":"+p.rport);
	}
      }
    }
    String[] bar=new String[foo.size()];
    for(int i=0; i<foo.size(); i++){
      bar[i]=(String)(foo.elementAt(i));
    }
    return bar;
  }
  static PortWatcher getPort(Session session, String address, int lport) throws JSchException{
    InetAddress addr;
    try{
      addr=InetAddress.getByName(address);
    }
    catch(UnknownHostException uhe){
      throw new JSchException("PortForwardingL: invalid address "+address+" specified.", uhe);
    }
    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
	PortWatcher p=(PortWatcher)(pool.elementAt(i));
	if(p.session==session && p.lport==lport){
	  if(/*p.boundaddress.isAnyLocalAddress() ||*/
             (anyLocalAddress!=null &&  p.boundaddress.equals(anyLocalAddress)) ||
	     p.boundaddress.equals(addr))
	  return p;
	}
      }
      return null;
    }
  }
  private static String normalize(String address){
    if(address!=null){
      if(address.length()==0 || address.equals("*"))
        address="0.0.0.0";
      else if(address.equals("localhost"))
        address="127.0.0.1";
    }
    return address;
  }
  static PortWatcher addPort(Session session, String address, int lport, String host, int rport, ServerSocketFactory ssf) throws JSchException{
    address = normalize(address);
    if(getPort(session, address, lport)!=null){
      throw new JSchException("PortForwardingL: local port "+ address+":"+lport+" is already registered.");
    }
    PortWatcher pw=new PortWatcher(session, address, lport, host, rport, ssf);
    pool.addElement(pw);
    return pw;
  }
  static void delPort(Session session, String address, int lport) throws JSchException{
    address = normalize(address);
    PortWatcher pw=getPort(session, address, lport);
    if(pw==null){
      throw new JSchException("PortForwardingL: local port "+address+":"+lport+" is not registered.");
    }
    pw.delete();
    pool.removeElement(pw);
  }
  static void delPort(Session session){
    synchronized(pool){
      PortWatcher[] foo=new PortWatcher[pool.size()];
      int count=0;
      for(int i=0; i<pool.size(); i++){
	PortWatcher p=(PortWatcher)(pool.elementAt(i));
	if(p.session==session) {
	  p.delete();
	  foo[count++]=p;
	}
      }
      for(int i=0; i<count; i++){
	PortWatcher p=foo[i];
	pool.removeElement(p);
      }
    }
  }
  PortWatcher(Session session, 
	      String address, int lport, 
	      String host, int rport,
              ServerSocketFactory factory) throws JSchException{
    this.session=session;
    this.lport=lport;
    this.host=host;
    this.rport=rport;
    try{
      boundaddress=InetAddress.getByName(address);
      ss=(factory==null) ? 
        new ServerSocket(lport, 0, boundaddress) :
        factory.createServerSocket(lport, 0, boundaddress);
    }
    catch(Exception e){ 
      //System.err.println(e);
      String message="PortForwardingL: local port "+address+":"+lport+" cannot be bound.";
      if(e instanceof Throwable)
        throw new JSchException(message, (Throwable)e);
      throw new JSchException(message);
    }
    if(lport==0){
      int assigned=ss.getLocalPort();
      if(assigned!=-1)
        this.lport=assigned;
    }
  }

  public void run(){
    thread=this;
    try{
      while(thread!=null){
        Socket socket=ss.accept();
	socket.setTcpNoDelay(true);
        InputStream in=socket.getInputStream();
        OutputStream out=socket.getOutputStream();
        ChannelDirectTCPIP channel=new ChannelDirectTCPIP();
        channel.init();
        channel.setInputStream(in);
        channel.setOutputStream(out);
	session.addChannel(channel);
	((ChannelDirectTCPIP)channel).setHost(host);
	((ChannelDirectTCPIP)channel).setPort(rport);
	((ChannelDirectTCPIP)channel).setOrgIPAddress(socket.getInetAddress().getHostAddress());
	((ChannelDirectTCPIP)channel).setOrgPort(socket.getPort());
        channel.connect(connectTimeout);
	if(channel.exitstatus!=-1){
	}
      }
    }
    catch(Exception e){
      //System.err.println("! "+e);
    }
    delete();
  }

  void delete(){
    thread=null;
    try{ 
      if(ss!=null)ss.close();
      ss=null;
    }
    catch(Exception e){
    }
  }

  void setConnectTimeout(int connectTimeout){
    this.connectTimeout=connectTimeout;
  }
}
