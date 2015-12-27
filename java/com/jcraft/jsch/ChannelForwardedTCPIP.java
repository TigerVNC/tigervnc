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
import java.util.Vector;

public class ChannelForwardedTCPIP extends Channel{

  private static Vector pool = new Vector();

  static private final int LOCAL_WINDOW_SIZE_MAX=0x20000;
//static private final int LOCAL_WINDOW_SIZE_MAX=0x100000;
  static private final int LOCAL_MAXIMUM_PACKET_SIZE=0x4000;

  static private final int TIMEOUT=10*1000;

  private Socket socket=null;
  private ForwardedTCPIPDaemon daemon=null;
  private Config config = null;

  ChannelForwardedTCPIP(){
    super();
    setLocalWindowSizeMax(LOCAL_WINDOW_SIZE_MAX);
    setLocalWindowSize(LOCAL_WINDOW_SIZE_MAX);
    setLocalPacketSize(LOCAL_MAXIMUM_PACKET_SIZE);
    io=new IO();
    connected=true;
  }

  public void run(){
    try{ 
      if(config instanceof ConfigDaemon){
        ConfigDaemon _config = (ConfigDaemon)config;
        Class c=Class.forName(_config.target);
        daemon=(ForwardedTCPIPDaemon)c.newInstance();

        PipedOutputStream out=new PipedOutputStream();
        io.setInputStream(new PassiveInputStream(out
                                                 , 32*1024
                                                 ), false);

        daemon.setChannel(this, getInputStream(), out);
        daemon.setArg(_config.arg);
        new Thread(daemon).start();
      }
      else{
        ConfigLHost _config = (ConfigLHost)config;
        socket=(_config.factory==null) ? 
           Util.createSocket(_config.target, _config.lport, TIMEOUT) : 
          _config.factory.createSocket(_config.target, _config.lport);
        socket.setTcpNoDelay(true);
        io.setInputStream(socket.getInputStream());
        io.setOutputStream(socket.getOutputStream());
      }
      sendOpenConfirmation();
    }
    catch(Exception e){
      sendOpenFailure(SSH_OPEN_ADMINISTRATIVELY_PROHIBITED);
      close=true;
      disconnect();
      return; 
    }

    thread=Thread.currentThread();
    Buffer buf=new Buffer(rmpsize);
    Packet packet=new Packet(buf);
    int i=0;
    try{
      Session _session = getSession();
      while(thread!=null && 
            io!=null && 
            io.in!=null){
        i=io.in.read(buf.buffer, 
                     14, 
                     buf.buffer.length-14
                     -Session.buffer_margin
                     );
        if(i<=0){
          eof();
          break;
        }
        packet.reset();
        buf.putByte((byte)Session.SSH_MSG_CHANNEL_DATA);
        buf.putInt(recipient);
        buf.putInt(i);
        buf.skip(i);
        synchronized(this){
          if(close)
            break;
          _session.write(packet, this, i);
        }
      }
    }
    catch(Exception e){
      //System.err.println(e);
    }
    //thread=null;
    //eof();
    disconnect();
  }

  void getData(Buffer buf){
    setRecipient(buf.getInt());
    setRemoteWindowSize(buf.getUInt());
    setRemotePacketSize(buf.getInt());
    byte[] addr=buf.getString();
    int port=buf.getInt();
    byte[] orgaddr=buf.getString();
    int orgport=buf.getInt();

    /*
    System.err.println("addr: "+Util.byte2str(addr));
    System.err.println("port: "+port);
    System.err.println("orgaddr: "+Util.byte2str(orgaddr));
    System.err.println("orgport: "+orgport);
    */

    Session _session=null;
    try{
      _session=getSession();
    }
    catch(JSchException e){
      // session has been already down.
    }

    this.config = getPort(_session, Util.byte2str(addr), port);
    if(this.config == null)
      this.config = getPort(_session, null, port);

    if(this.config == null){
      if(JSch.getLogger().isEnabled(Logger.ERROR)){
        JSch.getLogger().log(Logger.ERROR, 
                             "ChannelForwardedTCPIP: "+Util.byte2str(addr)+":"+port+" is not registered.");
      }
    }
  }

  private static Config getPort(Session session, String address_to_bind, int rport){
    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
        Config bar = (Config)(pool.elementAt(i));
        if(bar.session != session) continue;
        if(bar.rport != rport) {
          if(bar.rport != 0 || bar.allocated_rport != rport)
            continue;
        }
        if(address_to_bind != null &&
           !bar.address_to_bind.equals(address_to_bind)) continue;
        return bar;
      }
      return null;
    }
  }

  static String[] getPortForwarding(Session session){
    Vector foo = new Vector();
    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
        Config config = (Config)(pool.elementAt(i));
        if(config instanceof ConfigDaemon)
          foo.addElement(config.allocated_rport+":"+config.target+":");
        else
          foo.addElement(config.allocated_rport+":"+config.target+":"+((ConfigLHost)config).lport);
      }
    }
    String[] bar=new String[foo.size()];
    for(int i=0; i<foo.size(); i++){
      bar[i]=(String)(foo.elementAt(i));
    }
    return bar;
  }

  static String normalize(String address){
    if(address==null){ return "localhost"; }
    else if(address.length()==0 || address.equals("*")){ return ""; }
    else{ return address; }
  }

  static void addPort(Session session, String _address_to_bind,
                      int port, int allocated_port, String target, int lport, SocketFactory factory) throws JSchException{
    String address_to_bind=normalize(_address_to_bind);
    synchronized(pool){
      if(getPort(session, address_to_bind, port)!=null){
        throw new JSchException("PortForwardingR: remote port "+port+" is already registered.");
      }
      ConfigLHost config = new ConfigLHost();
      config.session = session;
      config.rport = port;
      config.allocated_rport = allocated_port;
      config.target = target;
      config.lport =lport;
      config.address_to_bind = address_to_bind;
      config.factory = factory;
      pool.addElement(config);
    }
  }
  static void addPort(Session session, String _address_to_bind,
                      int port, int allocated_port, String daemon, Object[] arg) throws JSchException{
    String address_to_bind=normalize(_address_to_bind);
    synchronized(pool){
      if(getPort(session, address_to_bind, port)!=null){
        throw new JSchException("PortForwardingR: remote port "+port+" is already registered.");
      }
      ConfigDaemon config = new ConfigDaemon();
      config.session = session;
      config.rport = port;
      config.allocated_rport = port;
      config.target = daemon;
      config.arg = arg;
      config.address_to_bind = address_to_bind;
      pool.addElement(config);
    }
  }
  static void delPort(ChannelForwardedTCPIP c){
    Session _session=null;
    try{
      _session=c.getSession();
    }
    catch(JSchException e){
      // session has been already down.
    }
    if(_session!=null && c.config!=null)
      delPort(_session, c.config.rport);
  }
  static void delPort(Session session, int rport){
    delPort(session, null, rport);
  }
  static void delPort(Session session, String address_to_bind, int rport){
    synchronized(pool){
      Config foo = getPort(session, normalize(address_to_bind), rport);
      if(foo == null)
        foo = getPort(session, null, rport);
      if(foo==null) return;
      pool.removeElement(foo);
      if(address_to_bind==null){
        address_to_bind=foo.address_to_bind;
      }	
      if(address_to_bind==null){
        address_to_bind="0.0.0.0";
      }
    }

    Buffer buf=new Buffer(100); // ??
    Packet packet=new Packet(buf);

    try{
      // byte SSH_MSG_GLOBAL_REQUEST 80
      // string "cancel-tcpip-forward"
      // boolean want_reply
      // string  address_to_bind (e.g. "127.0.0.1")
      // uint32  port number to bind
      packet.reset();
      buf.putByte((byte) 80/*SSH_MSG_GLOBAL_REQUEST*/);
      buf.putString(Util.str2byte("cancel-tcpip-forward"));
      buf.putByte((byte)0);
      buf.putString(Util.str2byte(address_to_bind));
      buf.putInt(rport);
      session.write(packet);
    }
    catch(Exception e){
//    throw new JSchException(e.toString());
    }
  }
  static void delPort(Session session){
    int[] rport=null;
    int count=0;
    synchronized(pool){
      rport=new int[pool.size()];
      for(int i=0; i<pool.size(); i++){
        Config config = (Config)(pool.elementAt(i));
        if(config.session == session) {
          rport[count++]=config.rport; // ((Integer)bar[1]).intValue();
        }
      }
    }
    for(int i=0; i<count; i++){
      delPort(session, rport[i]);
    }
  }

  public int getRemotePort(){return (config!=null ? config.rport: 0);}
  private void setSocketFactory(SocketFactory factory){
    if(config!=null && (config instanceof ConfigLHost) )
      ((ConfigLHost)config).factory = factory;
  }
  static abstract class Config {
    Session session;
    int rport;
    int allocated_rport;
    String address_to_bind;
    String target;
  }

  static class ConfigDaemon extends Config {
    Object[] arg;
  }

  static class ConfigLHost extends Config {
    int lport;
    SocketFactory factory;
  }
}
