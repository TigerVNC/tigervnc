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

public class Session implements Runnable{

  // http://ietf.org/internet-drafts/draft-ietf-secsh-assignednumbers-01.txt
  static final int SSH_MSG_DISCONNECT=                      1;
  static final int SSH_MSG_IGNORE=                          2;
  static final int SSH_MSG_UNIMPLEMENTED=                   3;
  static final int SSH_MSG_DEBUG=                           4;
  static final int SSH_MSG_SERVICE_REQUEST=                 5;
  static final int SSH_MSG_SERVICE_ACCEPT=                  6;
  static final int SSH_MSG_KEXINIT=                        20;
  static final int SSH_MSG_NEWKEYS=                        21;
  static final int SSH_MSG_KEXDH_INIT=                     30;
  static final int SSH_MSG_KEXDH_REPLY=                    31;
  static final int SSH_MSG_KEX_DH_GEX_GROUP=               31;
  static final int SSH_MSG_KEX_DH_GEX_INIT=                32;
  static final int SSH_MSG_KEX_DH_GEX_REPLY=               33;
  static final int SSH_MSG_KEX_DH_GEX_REQUEST=             34;
  static final int SSH_MSG_GLOBAL_REQUEST=                 80;
  static final int SSH_MSG_REQUEST_SUCCESS=                81;
  static final int SSH_MSG_REQUEST_FAILURE=                82;
  static final int SSH_MSG_CHANNEL_OPEN=                   90;
  static final int SSH_MSG_CHANNEL_OPEN_CONFIRMATION=      91;
  static final int SSH_MSG_CHANNEL_OPEN_FAILURE=           92;
  static final int SSH_MSG_CHANNEL_WINDOW_ADJUST=          93;
  static final int SSH_MSG_CHANNEL_DATA=                   94;
  static final int SSH_MSG_CHANNEL_EXTENDED_DATA=          95;
  static final int SSH_MSG_CHANNEL_EOF=                    96;
  static final int SSH_MSG_CHANNEL_CLOSE=                  97;
  static final int SSH_MSG_CHANNEL_REQUEST=                98;
  static final int SSH_MSG_CHANNEL_SUCCESS=                99;
  static final int SSH_MSG_CHANNEL_FAILURE=               100;

  private static final int PACKET_MAX_SIZE = 256 * 1024;

  private byte[] V_S;                                 // server version
  private byte[] V_C=Util.str2byte("SSH-2.0-JSCH-"+JSch.VERSION); // client version

  private byte[] I_C; // the payload of the client's SSH_MSG_KEXINIT
  private byte[] I_S; // the payload of the server's SSH_MSG_KEXINIT
  private byte[] K_S; // the host key

  private byte[] session_id;

  private byte[] IVc2s;
  private byte[] IVs2c;
  private byte[] Ec2s;
  private byte[] Es2c;
  private byte[] MACc2s;
  private byte[] MACs2c;

  private int seqi=0;
  private int seqo=0;

  String[] guess=null;
  private Cipher s2ccipher;
  private Cipher c2scipher;
  private MAC s2cmac;
  private MAC c2smac;
  //private byte[] mac_buf;
  private byte[] s2cmac_result1;
  private byte[] s2cmac_result2;

  private Compression deflater;
  private Compression inflater;

  private IO io;
  private Socket socket;
  private int timeout=0;

  private volatile boolean isConnected=false;

  private boolean isAuthed=false;

  private Thread connectThread=null;
  private Object lock=new Object();

  boolean x11_forwarding=false;
  boolean agent_forwarding=false;

  InputStream in=null;
  OutputStream out=null;

  static Random random;

  Buffer buf;
  Packet packet;

  SocketFactory socket_factory=null;

  static final int buffer_margin = 32 + // maximum padding length
                                   20 + // maximum mac length
                                   32;  // margin for deflater; deflater may inflate data

  private java.util.Hashtable config=null;

  private Proxy proxy=null;
  private UserInfo userinfo;

  private String hostKeyAlias=null;
  private int serverAliveInterval=0;
  private int serverAliveCountMax=1;

  protected boolean daemon_thread=false;

  private long kex_start_time=0L;

  int max_auth_tries = 6;
  int auth_failures = 0;

  String host="127.0.0.1";
  int port=22;

  String username=null;
  byte[] password=null;

  JSch jsch;

  Session(JSch jsch) throws JSchException{
    super();
    this.jsch=jsch;
    buf=new Buffer();
    packet=new Packet(buf);
  }

  public void connect() throws JSchException{
    connect(timeout);
  }

  public void connect(int connectTimeout) throws JSchException{
    if(isConnected){
      throw new JSchException("session is already connected");
    }

    io=new IO();
    if(random==null){
      try{
	Class c=Class.forName(getConfig("random"));
        random=(Random)(c.newInstance());
      }
      catch(Exception e){ 
        throw new JSchException(e.toString(), e);
      }
    }
    Packet.setRandom(random);

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO, 
                           "Connecting to "+host+" port "+port);
    }

    try	{
      int i, j;

      if(proxy==null){
        InputStream in;
        OutputStream out;
	if(socket_factory==null){
          socket=Util.createSocket(host, port, connectTimeout);
	  in=socket.getInputStream();
	  out=socket.getOutputStream();
	}
	else{
          socket=socket_factory.createSocket(host, port);
	  in=socket_factory.getInputStream(socket);
	  out=socket_factory.getOutputStream(socket);
	}
	//if(timeout>0){ socket.setSoTimeout(timeout); }
        socket.setTcpNoDelay(true);
        io.setInputStream(in);
        io.setOutputStream(out);
      }
      else{
	synchronized(proxy){
          proxy.connect(socket_factory, host, port, connectTimeout);
	  io.setInputStream(proxy.getInputStream());
	  io.setOutputStream(proxy.getOutputStream());
          socket=proxy.getSocket();
	}
      }

      if(connectTimeout>0 && socket!=null){
        socket.setSoTimeout(connectTimeout);
      }

      isConnected=true;

      if(JSch.getLogger().isEnabled(Logger.INFO)){
        JSch.getLogger().log(Logger.INFO, 
                             "Connection established");
      }

      jsch.addSession(this);

      {
	// Some Cisco devices will miss to read '\n' if it is sent separately.
	byte[] foo=new byte[V_C.length+1];
	System.arraycopy(V_C, 0, foo, 0, V_C.length);
	foo[foo.length-1]=(byte)'\n';
	io.put(foo, 0, foo.length);
      }

      while(true){
        i=0;
        j=0;
        while(i<buf.buffer.length){
          j=io.getByte();
          if(j<0)break;
          buf.buffer[i]=(byte)j; i++; 
          if(j==10)break;
        }
        if(j<0){
          throw new JSchException("connection is closed by foreign host");
        }

        if(buf.buffer[i-1]==10){    // 0x0a
          i--;
          if(i>0 && buf.buffer[i-1]==13){  // 0x0d
            i--;
          }
        }

        if(i<=3 || 
           ((i!=buf.buffer.length) &&
            (buf.buffer[0]!='S'||buf.buffer[1]!='S'||
             buf.buffer[2]!='H'||buf.buffer[3]!='-'))){
          // It must not start with 'SSH-'
          //System.err.println(new String(buf.buffer, 0, i);
          continue;
        }

        if(i==buf.buffer.length ||
           i<7 ||                                      // SSH-1.99 or SSH-2.0
           (buf.buffer[4]=='1' && buf.buffer[6]!='9')  // SSH-1.5
           ){
          throw new JSchException("invalid server's version string");
        }
        break;
      }

      V_S=new byte[i]; System.arraycopy(buf.buffer, 0, V_S, 0, i);
      //System.err.println("V_S: ("+i+") ["+new String(V_S)+"]");

      if(JSch.getLogger().isEnabled(Logger.INFO)){
        JSch.getLogger().log(Logger.INFO, 
                             "Remote version string: "+Util.byte2str(V_S));
        JSch.getLogger().log(Logger.INFO, 
                             "Local version string: "+Util.byte2str(V_C));
      }

      send_kexinit();

      buf=read(buf);
      if(buf.getCommand()!=SSH_MSG_KEXINIT){
        in_kex=false;
	throw new JSchException("invalid protocol: "+buf.getCommand());
      }

      if(JSch.getLogger().isEnabled(Logger.INFO)){
        JSch.getLogger().log(Logger.INFO, 
                             "SSH_MSG_KEXINIT received");
      }

      KeyExchange kex=receive_kexinit(buf);

      while(true){
	buf=read(buf);
	if(kex.getState()==buf.getCommand()){
          kex_start_time=System.currentTimeMillis();
          boolean result=kex.next(buf);
	  if(!result){
	    //System.err.println("verify: "+result);
            in_kex=false;
	    throw new JSchException("verify: "+result);
	  }
	}
	else{
          in_kex=false;
	  throw new JSchException("invalid protocol(kex): "+buf.getCommand());
	}
	if(kex.getState()==KeyExchange.STATE_END){
	  break;
	}
      }

      try{ checkHost(host, port, kex); }
      catch(JSchException ee){
        in_kex=false;
        throw ee;
      }

      send_newkeys();

      // receive SSH_MSG_NEWKEYS(21)
      buf=read(buf);
      //System.err.println("read: 21 ? "+buf.getCommand());
      if(buf.getCommand()==SSH_MSG_NEWKEYS){

        if(JSch.getLogger().isEnabled(Logger.INFO)){
          JSch.getLogger().log(Logger.INFO, 
                               "SSH_MSG_NEWKEYS received");
        }

	receive_newkeys(buf, kex);
      }
      else{
        in_kex=false;
	throw new JSchException("invalid protocol(newkyes): "+buf.getCommand());
      }

      try{
        String s = getConfig("MaxAuthTries");
        if(s!=null){
          max_auth_tries = Integer.parseInt(s);
        }
      }
      catch(NumberFormatException e){
        throw new JSchException("MaxAuthTries: "+getConfig("MaxAuthTries"), e);
      }

      boolean auth=false;
      boolean auth_cancel=false;

      UserAuth ua=null;
      try{
	Class c=Class.forName(getConfig("userauth.none"));
        ua=(UserAuth)(c.newInstance());
      }
      catch(Exception e){ 
        throw new JSchException(e.toString(), e);
      }

      auth=ua.start(this);

      String cmethods=getConfig("PreferredAuthentications");

      String[] cmethoda=Util.split(cmethods, ",");

      String smethods=null;
      if(!auth){
        smethods=((UserAuthNone)ua).getMethods();
        if(smethods!=null){
          smethods=smethods.toLowerCase();
        }
        else{
          // methods: publickey,password,keyboard-interactive
          //smethods="publickey,password,keyboard-interactive";
          smethods=cmethods;
        }
      }

      String[] smethoda=Util.split(smethods, ",");

      int methodi=0;

      loop:
      while(true){

	while(!auth && 
	      cmethoda!=null && methodi<cmethoda.length){

          String method=cmethoda[methodi++];
          boolean acceptable=false;
          for(int k=0; k<smethoda.length; k++){
            if(smethoda[k].equals(method)){
              acceptable=true;
              break;
            }
          }
          if(!acceptable){
            continue;
          }

          //System.err.println("  method: "+method);

          if(JSch.getLogger().isEnabled(Logger.INFO)){
            String str="Authentications that can continue: ";
            for(int k=methodi-1; k<cmethoda.length; k++){
              str+=cmethoda[k];
              if(k+1<cmethoda.length)
                str+=",";
            }
            JSch.getLogger().log(Logger.INFO, 
                                 str);
            JSch.getLogger().log(Logger.INFO, 
                                 "Next authentication method: "+method);
          }

	  ua=null;
          try{
            Class c=null;
            if(getConfig("userauth."+method)!=null){
              c=Class.forName(getConfig("userauth."+method));
              ua=(UserAuth)(c.newInstance());
            }
          }
          catch(Exception e){
            if(JSch.getLogger().isEnabled(Logger.WARN)){
              JSch.getLogger().log(Logger.WARN, 
                                   "failed to load "+method+" method");
            }
          }

	  if(ua!=null){
            auth_cancel=false;
	    try{ 
	      auth=ua.start(this); 
              if(auth && 
                 JSch.getLogger().isEnabled(Logger.INFO)){
                JSch.getLogger().log(Logger.INFO, 
                                     "Authentication succeeded ("+method+").");
              }
	    }
	    catch(JSchAuthCancelException ee){
	      auth_cancel=true;
	    }
	    catch(JSchPartialAuthException ee){
              String tmp = smethods;
              smethods=ee.getMethods();
              smethoda=Util.split(smethods, ",");
              if(!tmp.equals(smethods)){
                methodi=0;
              }
	      //System.err.println("PartialAuth: "+methods);
	      auth_cancel=false;
	      continue loop;
	    }
	    catch(RuntimeException ee){
	      throw ee;
	    }
	    catch(Exception ee){
	      //System.err.println("ee: "+ee); // SSH_MSG_DISCONNECT: 2 Too many authentication failures
              break loop;
	    }
	  }
	}
        break;
      }

      if(!auth){
        if(auth_failures >= max_auth_tries){
          if(JSch.getLogger().isEnabled(Logger.INFO)){
            JSch.getLogger().log(Logger.INFO, 
                                 "Login trials exceeds "+max_auth_tries);
          }
        }
        if(auth_cancel)
          throw new JSchException("Auth cancel");
        throw new JSchException("Auth fail");
      }

      if(connectTimeout>0 || timeout>0){
        socket.setSoTimeout(timeout);
      }

      isAuthed=true;

      synchronized(lock){
        if(isConnected){
          connectThread=new Thread(this);
          connectThread.setName("Connect thread "+host+" session");
          if(daemon_thread){
            connectThread.setDaemon(daemon_thread);
          }
          connectThread.start();
        }
        else{
          // The session has been already down and
          // we don't have to start new thread.
        }
      }
    }
    catch(Exception e) {
      in_kex=false;
      if(isConnected){
	try{
	  packet.reset();
	  buf.putByte((byte)SSH_MSG_DISCONNECT);
	  buf.putInt(3);
	  buf.putString(Util.str2byte(e.toString()));
	  buf.putString(Util.str2byte("en"));
	  write(packet);
	  disconnect();
	}
	catch(Exception ee){
	}
      }
      isConnected=false;
      //e.printStackTrace();
      if(e instanceof RuntimeException) throw (RuntimeException)e;
      if(e instanceof JSchException) throw (JSchException)e;
      throw new JSchException("Session.connect: "+e);
    }
    finally{
      Util.bzero(this.password);
      this.password=null;
    }
  }

  private KeyExchange receive_kexinit(Buffer buf) throws Exception {
    int j=buf.getInt();
    if(j!=buf.getLength()){    // packet was compressed and
      buf.getByte();           // j is the size of deflated packet.
      I_S=new byte[buf.index-5];
    }
    else{
      I_S=new byte[j-1-buf.getByte()];
    }
   System.arraycopy(buf.buffer, buf.s, I_S, 0, I_S.length);

   if(!in_kex){     // We are in rekeying activated by the remote!
     send_kexinit();
   }

    guess=KeyExchange.guess(I_S, I_C);
    if(guess==null){
      throw new JSchException("Algorithm negotiation fail");
    }

    if(!isAuthed &&
       (guess[KeyExchange.PROPOSAL_ENC_ALGS_CTOS].equals("none") ||
        (guess[KeyExchange.PROPOSAL_ENC_ALGS_STOC].equals("none")))){
      throw new JSchException("NONE Cipher should not be chosen before authentification is successed.");
    }

    KeyExchange kex=null;
    try{
      Class c=Class.forName(getConfig(guess[KeyExchange.PROPOSAL_KEX_ALGS]));
      kex=(KeyExchange)(c.newInstance());
    }
    catch(Exception e){ 
      throw new JSchException(e.toString(), e);
    }

    kex.init(this, V_S, V_C, I_S, I_C);
    return kex;
  }

  private boolean in_kex=false;
  public void rekey() throws Exception {
    send_kexinit();
  }
  private void send_kexinit() throws Exception {
    if(in_kex)
      return;

    String cipherc2s=getConfig("cipher.c2s");
    String ciphers2c=getConfig("cipher.s2c");

    String[] not_available_ciphers=checkCiphers(getConfig("CheckCiphers"));
    if(not_available_ciphers!=null && not_available_ciphers.length>0){
      cipherc2s=Util.diffString(cipherc2s, not_available_ciphers);
      ciphers2c=Util.diffString(ciphers2c, not_available_ciphers);
      if(cipherc2s==null || ciphers2c==null){
        throw new JSchException("There are not any available ciphers.");
      }
    }

    String kex=getConfig("kex");
    String[] not_available_kexes=checkKexes(getConfig("CheckKexes"));
    if(not_available_kexes!=null && not_available_kexes.length>0){
      kex=Util.diffString(kex, not_available_kexes);
      if(kex==null){
        throw new JSchException("There are not any available kexes.");
      }
    }

    in_kex=true;
    kex_start_time=System.currentTimeMillis();

    // byte      SSH_MSG_KEXINIT(20)
    // byte[16]  cookie (random bytes)
    // string    kex_algorithms
    // string    server_host_key_algorithms
    // string    encryption_algorithms_client_to_server
    // string    encryption_algorithms_server_to_client
    // string    mac_algorithms_client_to_server
    // string    mac_algorithms_server_to_client
    // string    compression_algorithms_client_to_server
    // string    compression_algorithms_server_to_client
    // string    languages_client_to_server
    // string    languages_server_to_client
    Buffer buf = new Buffer();                // send_kexinit may be invoked
    Packet packet = new Packet(buf);          // by user thread.
    packet.reset();
    buf.putByte((byte) SSH_MSG_KEXINIT);
    synchronized(random){
      random.fill(buf.buffer, buf.index, 16); buf.skip(16);
    }
    buf.putString(Util.str2byte(kex));
    buf.putString(Util.str2byte(getConfig("server_host_key")));
    buf.putString(Util.str2byte(cipherc2s));
    buf.putString(Util.str2byte(ciphers2c));
    buf.putString(Util.str2byte(getConfig("mac.c2s")));
    buf.putString(Util.str2byte(getConfig("mac.s2c")));
    buf.putString(Util.str2byte(getConfig("compression.c2s")));
    buf.putString(Util.str2byte(getConfig("compression.s2c")));
    buf.putString(Util.str2byte(getConfig("lang.c2s")));
    buf.putString(Util.str2byte(getConfig("lang.s2c")));
    buf.putByte((byte)0);
    buf.putInt(0);

    buf.setOffSet(5);
    I_C=new byte[buf.getLength()];
    buf.getByte(I_C);

    write(packet);

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO, 
                           "SSH_MSG_KEXINIT sent");
    }
  }

  private void send_newkeys() throws Exception {
    // send SSH_MSG_NEWKEYS(21)
    packet.reset();
    buf.putByte((byte)SSH_MSG_NEWKEYS);
    write(packet);

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO, 
                           "SSH_MSG_NEWKEYS sent");
    }
  }

  private void checkHost(String chost, int port, KeyExchange kex) throws JSchException {
    String shkc=getConfig("StrictHostKeyChecking");

    if(hostKeyAlias!=null){
      chost=hostKeyAlias;
    }

    //System.err.println("shkc: "+shkc);

    byte[] K_S=kex.getHostKey();
    String key_type=kex.getKeyType();
    String key_fprint=kex.getFingerPrint();

    if(hostKeyAlias==null && port!=22){
      chost=("["+chost+"]:"+port);
    }

//    hostkey=new HostKey(chost, K_S);

    HostKeyRepository hkr=jsch.getHostKeyRepository();
    int i=0;
    synchronized(hkr){
      i=hkr.check(chost, K_S);
    }

    boolean insert=false;

    if((shkc.equals("ask") || shkc.equals("yes")) &&
       i==HostKeyRepository.CHANGED){
      String file=null;
      synchronized(hkr){
	file=hkr.getKnownHostsRepositoryID();
      }
      if(file==null){file="known_hosts";}

      boolean b=false;

      if(userinfo!=null){
        String message=
"WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!\n"+
"IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!\n"+
"Someone could be eavesdropping on you right now (man-in-the-middle attack)!\n"+
"It is also possible that the "+key_type+" host key has just been changed.\n"+
"The fingerprint for the "+key_type+" key sent by the remote host is\n"+
key_fprint+".\n"+
"Please contact your system administrator.\n"+
"Add correct host key in "+file+" to get rid of this message.";

        if(shkc.equals("ask")){
          b=userinfo.promptYesNo(message+
                                 "\nDo you want to delete the old key and insert the new key?");
        }
        else{  // shkc.equals("yes")
          userinfo.showMessage(message);
        }
      }

      if(!b){
        throw new JSchException("HostKey has been changed: "+chost);
      }

      synchronized(hkr){
        hkr.remove(chost, 
                   (key_type.equals("DSA") ? "ssh-dss" : "ssh-rsa"), 
                   null);
        insert=true;
      }
    }

    if((shkc.equals("ask") || shkc.equals("yes")) &&
       (i!=HostKeyRepository.OK) && !insert){
      if(shkc.equals("yes")){
	throw new JSchException("reject HostKey: "+host);
      }
      //System.err.println("finger-print: "+key_fprint);
      if(userinfo!=null){
	boolean foo=userinfo.promptYesNo(
"The authenticity of host '"+host+"' can't be established.\n"+
key_type+" key fingerprint is "+key_fprint+".\n"+
"Are you sure you want to continue connecting?"
					 );
	if(!foo){
	  throw new JSchException("reject HostKey: "+host);
	}
	insert=true;
      }
      else{
	if(i==HostKeyRepository.NOT_INCLUDED) 
	  throw new JSchException("UnknownHostKey: "+host+". "+key_type+" key fingerprint is "+key_fprint);
	else 
          throw new JSchException("HostKey has been changed: "+host);
      }
    }

    if(shkc.equals("no") && 
       HostKeyRepository.NOT_INCLUDED==i){
      insert=true;
    }

    if(i==HostKeyRepository.OK &&
       JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO, 
                           "Host '"+host+"' is known and mathces the "+key_type+" host key");
    }

    if(insert &&
       JSch.getLogger().isEnabled(Logger.WARN)){
      JSch.getLogger().log(Logger.WARN, 
                           "Permanently added '"+host+"' ("+key_type+") to the list of known hosts.");
    }

    String hkh=getConfig("HashKnownHosts");
    if(hkh.equals("yes") && (hkr instanceof KnownHosts)){
      hostkey=((KnownHosts)hkr).createHashedHostKey(chost, K_S);
    }
    else{
      hostkey=new HostKey(chost, K_S);
    }

    if(insert){
      synchronized(hkr){
	hkr.add(hostkey, userinfo);
      }

    }

  }

//public void start(){ (new Thread(this)).start();  }

  public Channel openChannel(String type) throws JSchException{
    if(!isConnected){
      throw new JSchException("session is down");
    }
    try{
      Channel channel=Channel.getChannel(type);
      addChannel(channel);
      channel.init();
      return channel;
    }
    catch(Exception e){
      //e.printStackTrace();
    }
    return null;
  }

  // encode will bin invoked in write with synchronization.
  public void encode(Packet packet) throws Exception{
//System.err.println("encode: "+packet.buffer.getCommand());
//System.err.println("        "+packet.buffer.index);
//if(packet.buffer.getCommand()==96){
//Thread.dumpStack();
//}
    if(deflater!=null){
      compress_len[0]=packet.buffer.index;
      packet.buffer.buffer=deflater.compress(packet.buffer.buffer, 
                                             5, compress_len);
      packet.buffer.index=compress_len[0];
    }
    if(c2scipher!=null){
      //packet.padding(c2scipher.getIVSize());
      packet.padding(c2scipher_size);
      int pad=packet.buffer.buffer[4];
      synchronized(random){
	random.fill(packet.buffer.buffer, packet.buffer.index-pad, pad);
      }
    }
    else{
      packet.padding(8);
    }

    if(c2smac!=null){
      c2smac.update(seqo);
      c2smac.update(packet.buffer.buffer, 0, packet.buffer.index);
      c2smac.doFinal(packet.buffer.buffer, packet.buffer.index);
    }
    if(c2scipher!=null){
      byte[] buf=packet.buffer.buffer;
      c2scipher.update(buf, 0, packet.buffer.index, buf, 0);
    }
    if(c2smac!=null){
      packet.buffer.skip(c2smac.getBlockSize());
    }
  }

  int[] uncompress_len=new int[1];
  int[] compress_len=new int[1];

  private int s2ccipher_size=8;
  private int c2scipher_size=8;
  public Buffer read(Buffer buf) throws Exception{
    int j=0;
    while(true){
      buf.reset();
      io.getByte(buf.buffer, buf.index, s2ccipher_size); 
      buf.index+=s2ccipher_size;
      if(s2ccipher!=null){
        s2ccipher.update(buf.buffer, 0, s2ccipher_size, buf.buffer, 0);
      }
      j=((buf.buffer[0]<<24)&0xff000000)|
        ((buf.buffer[1]<<16)&0x00ff0000)|
        ((buf.buffer[2]<< 8)&0x0000ff00)|
        ((buf.buffer[3]    )&0x000000ff);
      // RFC 4253 6.1. Maximum Packet Length
      if(j<5 || j>PACKET_MAX_SIZE){
        start_discard(buf, s2ccipher, s2cmac, j, PACKET_MAX_SIZE);
      }
      int need = j+4-s2ccipher_size;
      //if(need<0){
      //  throw new IOException("invalid data");
      //}
      if((buf.index+need)>buf.buffer.length){
        byte[] foo=new byte[buf.index+need];
        System.arraycopy(buf.buffer, 0, foo, 0, buf.index);
        buf.buffer=foo;
      }

      if((need%s2ccipher_size)!=0){
        String message="Bad packet length "+need;
        if(JSch.getLogger().isEnabled(Logger.FATAL)){
          JSch.getLogger().log(Logger.FATAL, message); 
        }
        start_discard(buf, s2ccipher, s2cmac, j, PACKET_MAX_SIZE-s2ccipher_size);
      }

      if(need>0){
	io.getByte(buf.buffer, buf.index, need); buf.index+=(need);
	if(s2ccipher!=null){
	  s2ccipher.update(buf.buffer, s2ccipher_size, need, buf.buffer, s2ccipher_size);
	}
      }

      if(s2cmac!=null){
	s2cmac.update(seqi);
	s2cmac.update(buf.buffer, 0, buf.index);

        s2cmac.doFinal(s2cmac_result1, 0);
	io.getByte(s2cmac_result2, 0, s2cmac_result2.length);
        if(!java.util.Arrays.equals(s2cmac_result1, s2cmac_result2)){
          if(need > PACKET_MAX_SIZE){
            throw new IOException("MAC Error");
          }
          start_discard(buf, s2ccipher, s2cmac, j, PACKET_MAX_SIZE-need);
          continue;
	}
      }

      seqi++;

      if(inflater!=null){
        //inflater.uncompress(buf);
	int pad=buf.buffer[4];
	uncompress_len[0]=buf.index-5-pad;
	byte[] foo=inflater.uncompress(buf.buffer, 5, uncompress_len);
	if(foo!=null){
	  buf.buffer=foo;
	  buf.index=5+uncompress_len[0];
	}
	else{
	  System.err.println("fail in inflater");
	  break;
	}
      }

      int type=buf.getCommand()&0xff;
      //System.err.println("read: "+type);
      if(type==SSH_MSG_DISCONNECT){
        buf.rewind();
        buf.getInt();buf.getShort();
	int reason_code=buf.getInt();
	byte[] description=buf.getString();
	byte[] language_tag=buf.getString();
	throw new JSchException("SSH_MSG_DISCONNECT: "+
				    reason_code+
				" "+Util.byte2str(description)+
				" "+Util.byte2str(language_tag));
	//break;
      }
      else if(type==SSH_MSG_IGNORE){
      }
      else if(type==SSH_MSG_UNIMPLEMENTED){
        buf.rewind();
        buf.getInt();buf.getShort();
	int reason_id=buf.getInt();
        if(JSch.getLogger().isEnabled(Logger.INFO)){
          JSch.getLogger().log(Logger.INFO, 
                               "Received SSH_MSG_UNIMPLEMENTED for "+reason_id);
        }
      }
      else if(type==SSH_MSG_DEBUG){
        buf.rewind();
        buf.getInt();buf.getShort();
/*
	byte always_display=(byte)buf.getByte();
	byte[] message=buf.getString();
	byte[] language_tag=buf.getString();
	System.err.println("SSH_MSG_DEBUG:"+
			   " "+Util.byte2str(message)+
			   " "+Util.byte2str(language_tag));
*/
      }
      else if(type==SSH_MSG_CHANNEL_WINDOW_ADJUST){
          buf.rewind();
          buf.getInt();buf.getShort();
	  Channel c=Channel.getChannel(buf.getInt(), this);
	  if(c==null){
	  }
	  else{
	    c.addRemoteWindowSize(buf.getInt()); 
	  }
      }
      else if(type==UserAuth.SSH_MSG_USERAUTH_SUCCESS){
        isAuthed=true;
        if(inflater==null && deflater==null){
          String method;
          method=guess[KeyExchange.PROPOSAL_COMP_ALGS_CTOS];
          initDeflater(method);
          method=guess[KeyExchange.PROPOSAL_COMP_ALGS_STOC];
          initInflater(method);
        }
        break;
      }
      else{
        break;
      }
    }
    buf.rewind();
    return buf;
  }

  private void start_discard(Buffer buf, Cipher cipher, MAC mac, 
                             int packet_length, int discard) throws JSchException, IOException{
    MAC discard_mac = null;

    if(!cipher.isCBC()){
      throw new JSchException("Packet corrupt");
    }

    if(packet_length!=PACKET_MAX_SIZE && mac != null){
      discard_mac = mac;
    }

    discard -= buf.index;

    while(discard>0){
      buf.reset();
      int len = discard>buf.buffer.length ? buf.buffer.length : discard;
      io.getByte(buf.buffer, 0, len);
      if(discard_mac!=null){
        discard_mac.update(buf.buffer, 0, len);
      }
      discard -= len;
    }

    if(discard_mac!=null){
      discard_mac.doFinal(buf.buffer, 0);
    }

    throw new JSchException("Packet corrupt");
  }

  byte[] getSessionId(){
    return session_id;
  }

  private void receive_newkeys(Buffer buf, KeyExchange kex) throws Exception {
    updateKeys(kex);
    in_kex=false;
  }
  private void updateKeys(KeyExchange kex) throws Exception{
    byte[] K=kex.getK();
    byte[] H=kex.getH();
    HASH hash=kex.getHash();

//    String[] guess=kex.guess;

    if(session_id==null){
      session_id=new byte[H.length];
      System.arraycopy(H, 0, session_id, 0, H.length);
    }

    /*
      Initial IV client to server:     HASH (K || H || "A" || session_id)
      Initial IV server to client:     HASH (K || H || "B" || session_id)
      Encryption key client to server: HASH (K || H || "C" || session_id)
      Encryption key server to client: HASH (K || H || "D" || session_id)
      Integrity key client to server:  HASH (K || H || "E" || session_id)
      Integrity key server to client:  HASH (K || H || "F" || session_id)
    */

    buf.reset();
    buf.putMPInt(K);
    buf.putByte(H);
    buf.putByte((byte)0x41);
    buf.putByte(session_id);
    hash.update(buf.buffer, 0, buf.index);
    IVc2s=hash.digest();

    int j=buf.index-session_id.length-1;

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    IVs2c=hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    Ec2s=hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    Es2c=hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    MACc2s=hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    MACs2c=hash.digest();

    try{
      Class c;
      String method;
  
      method=guess[KeyExchange.PROPOSAL_ENC_ALGS_STOC];
      c=Class.forName(getConfig(method));
      s2ccipher=(Cipher)(c.newInstance());
      while(s2ccipher.getBlockSize()>Es2c.length){
        buf.reset();
        buf.putMPInt(K);
        buf.putByte(H);
        buf.putByte(Es2c);
        hash.update(buf.buffer, 0, buf.index);
        byte[] foo=hash.digest();
        byte[] bar=new byte[Es2c.length+foo.length];
	System.arraycopy(Es2c, 0, bar, 0, Es2c.length);
	System.arraycopy(foo, 0, bar, Es2c.length, foo.length);
	Es2c=bar;
      }
      s2ccipher.init(Cipher.DECRYPT_MODE, Es2c, IVs2c);
      s2ccipher_size=s2ccipher.getIVSize();

      method=guess[KeyExchange.PROPOSAL_MAC_ALGS_STOC];
      c=Class.forName(getConfig(method));
      s2cmac=(MAC)(c.newInstance());
      s2cmac.init(MACs2c);
      //mac_buf=new byte[s2cmac.getBlockSize()];
      s2cmac_result1=new byte[s2cmac.getBlockSize()];
      s2cmac_result2=new byte[s2cmac.getBlockSize()];

      method=guess[KeyExchange.PROPOSAL_ENC_ALGS_CTOS];
      c=Class.forName(getConfig(method));
      c2scipher=(Cipher)(c.newInstance());
      while(c2scipher.getBlockSize()>Ec2s.length){
        buf.reset();
        buf.putMPInt(K);
        buf.putByte(H);
        buf.putByte(Ec2s);
        hash.update(buf.buffer, 0, buf.index);
        byte[] foo=hash.digest();
        byte[] bar=new byte[Ec2s.length+foo.length];
	System.arraycopy(Ec2s, 0, bar, 0, Ec2s.length);
	System.arraycopy(foo, 0, bar, Ec2s.length, foo.length);
	Ec2s=bar;
      }
      c2scipher.init(Cipher.ENCRYPT_MODE, Ec2s, IVc2s);
      c2scipher_size=c2scipher.getIVSize();

      method=guess[KeyExchange.PROPOSAL_MAC_ALGS_CTOS];
      c=Class.forName(getConfig(method));
      c2smac=(MAC)(c.newInstance());
      c2smac.init(MACc2s);

      method=guess[KeyExchange.PROPOSAL_COMP_ALGS_CTOS];
      initDeflater(method);

      method=guess[KeyExchange.PROPOSAL_COMP_ALGS_STOC];
      initInflater(method);
    }
    catch(Exception e){ 
      if(e instanceof JSchException)
        throw e;
      throw new JSchException(e.toString(), e);
      //System.err.println("updatekeys: "+e); 
    }
  }

  /*public*/ /*synchronized*/ void write(Packet packet, Channel c, int length) throws Exception{
    long t = getTimeout();
    while(true){
      if(in_kex){
        if(t>0L && (System.currentTimeMillis()-kex_start_time)>t){
          throw new JSchException("timeout in wating for rekeying process.");
        }
        try{Thread.sleep(10);}
        catch(java.lang.InterruptedException e){};
        continue;
      }
      synchronized(c){

        if(c.rwsize<length){
          try{ 
            c.notifyme++;
            c.wait(100); 
          }
          catch(java.lang.InterruptedException e){
          }
          finally{
            c.notifyme--;
          }
        }

        if(c.rwsize>=length){
          c.rwsize-=length;
          break;
        }

      }
      if(c.close || !c.isConnected()){
	throw new IOException("channel is broken");
      }

      boolean sendit=false;
      int s=0;
      byte command=0;
      int recipient=-1;
      synchronized(c){
	if(c.rwsize>0){
	  long len=c.rwsize;
          if(len>length){
            len=length;
          }
          if(len!=length){
            s=packet.shift((int)len, 
                           (c2scipher!=null ? c2scipher_size : 8),
                           (c2smac!=null ? c2smac.getBlockSize() : 0));
          }
	  command=packet.buffer.getCommand();
	  recipient=c.getRecipient();
	  length-=len;
	  c.rwsize-=len;
	  sendit=true;
	}
      }
      if(sendit){
	_write(packet);
        if(length==0){
          return;
        }
	packet.unshift(command, recipient, s, length);
      }

      synchronized(c){
        if(in_kex){
          continue;
        }
        if(c.rwsize>=length){
          c.rwsize-=length;
          break;
        }

        //try{ 
        //System.out.println("1wait: "+c.rwsize);
        //  c.notifyme++;
        //  c.wait(100); 
        //}
        //catch(java.lang.InterruptedException e){
        //}
        //finally{
        //  c.notifyme--;
        //}
      }
    }
    _write(packet);
  }

  public void write(Packet packet) throws Exception{
    // System.err.println("in_kex="+in_kex+" "+(packet.buffer.getCommand()));
    long t = getTimeout();
    while(in_kex){
      if(t>0L && (System.currentTimeMillis()-kex_start_time)>t){
        throw new JSchException("timeout in wating for rekeying process.");
      }
      byte command=packet.buffer.getCommand();
      //System.err.println("command: "+command);
      if(command==SSH_MSG_KEXINIT ||
         command==SSH_MSG_NEWKEYS ||
         command==SSH_MSG_KEXDH_INIT ||
         command==SSH_MSG_KEXDH_REPLY ||
         command==SSH_MSG_KEX_DH_GEX_GROUP ||
         command==SSH_MSG_KEX_DH_GEX_INIT ||
         command==SSH_MSG_KEX_DH_GEX_REPLY ||
         command==SSH_MSG_KEX_DH_GEX_REQUEST ||
         command==SSH_MSG_DISCONNECT){
        break;
      }
      try{Thread.sleep(10);}
      catch(java.lang.InterruptedException e){};
    }
    _write(packet);
  }

  private void _write(Packet packet) throws Exception{
    synchronized(lock){
      encode(packet);
      if(io!=null){
        io.put(packet);
        seqo++;
      }
    }
  }

  Runnable thread;
  public void run(){
    thread=this;

    byte[] foo;
    Buffer buf=new Buffer();
    Packet packet=new Packet(buf);
    int i=0;
    Channel channel;
    int[] start=new int[1];
    int[] length=new int[1];
    KeyExchange kex=null;

    int stimeout=0;
    try{
      while(isConnected &&
	    thread!=null){
        try{
          buf=read(buf);
          stimeout=0;
        }
        catch(InterruptedIOException/*SocketTimeoutException*/ ee){
          if(!in_kex && stimeout<serverAliveCountMax){
            sendKeepAliveMsg();
            stimeout++;
            continue;
          }
          else if(in_kex && stimeout<serverAliveCountMax){
            stimeout++;
            continue;
          }
          throw ee;
        }

	int msgType=buf.getCommand()&0xff;

	if(kex!=null && kex.getState()==msgType){
          kex_start_time=System.currentTimeMillis();
	  boolean result=kex.next(buf);
	  if(!result){
	    throw new JSchException("verify: "+result);
	  }
	  continue;
	}

        switch(msgType){
	case SSH_MSG_KEXINIT:
//System.err.println("KEXINIT");
	  kex=receive_kexinit(buf);
	  break;

	case SSH_MSG_NEWKEYS:
//System.err.println("NEWKEYS");
          send_newkeys();
	  receive_newkeys(buf, kex);
	  kex=null;
	  break;

	case SSH_MSG_CHANNEL_DATA:
          buf.getInt(); 
          buf.getByte(); 
          buf.getByte(); 
          i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  foo=buf.getString(start, length);
	  if(channel==null){
	    break;
	  }

          if(length[0]==0){
	    break;
          }

try{
	  channel.write(foo, start[0], length[0]);
}
catch(Exception e){
//System.err.println(e);
  try{channel.disconnect();}catch(Exception ee){}
break;
}
	  int len=length[0];
	  channel.setLocalWindowSize(channel.lwsize-len);
 	  if(channel.lwsize<channel.lwsize_max/2){
            packet.reset();
	    buf.putByte((byte)SSH_MSG_CHANNEL_WINDOW_ADJUST);
	    buf.putInt(channel.getRecipient());
	    buf.putInt(channel.lwsize_max-channel.lwsize);
            synchronized(channel){
              if(!channel.close)
                write(packet);
            }
	    channel.setLocalWindowSize(channel.lwsize_max);
	  }
	  break;

        case SSH_MSG_CHANNEL_EXTENDED_DATA:
          buf.getInt();
	  buf.getShort();
	  i=buf.getInt();
	  channel=Channel.getChannel(i, this);
	  buf.getInt();                   // data_type_code == 1
	  foo=buf.getString(start, length);
	  //System.err.println("stderr: "+new String(foo,start[0],length[0]));
	  if(channel==null){
	    break;
	  }

          if(length[0]==0){
	    break;
          }

	  channel.write_ext(foo, start[0], length[0]);

	  len=length[0];
	  channel.setLocalWindowSize(channel.lwsize-len);
 	  if(channel.lwsize<channel.lwsize_max/2){
            packet.reset();
	    buf.putByte((byte)SSH_MSG_CHANNEL_WINDOW_ADJUST);
	    buf.putInt(channel.getRecipient());
	    buf.putInt(channel.lwsize_max-channel.lwsize);
            synchronized(channel){
              if(!channel.close)
                write(packet);
            }
	    channel.setLocalWindowSize(channel.lwsize_max);
	  }
	  break;

	case SSH_MSG_CHANNEL_WINDOW_ADJUST:
          buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel==null){
	    break;
	  }
	  channel.addRemoteWindowSize(buf.getInt()); 
	  break;

	case SSH_MSG_CHANNEL_EOF:
          buf.getInt(); 
          buf.getShort(); 
          i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel!=null){
	    //channel.eof_remote=true;
	    //channel.eof();
	    channel.eof_remote();
	  }
	  /*
	  packet.reset();
	  buf.putByte((byte)SSH_MSG_CHANNEL_EOF);
	  buf.putInt(channel.getRecipient());
	  write(packet);
	  */
	  break;
	case SSH_MSG_CHANNEL_CLOSE:
          buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel!=null){
//	      channel.close();
	    channel.disconnect();
	  }
	  /*
          if(Channel.pool.size()==0){
	    thread=null;
	  }
	  */
	  break;
	case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
          buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel==null){
	    //break;
	  }
          int r=buf.getInt();
          long rws=buf.getUInt();
          int rps=buf.getInt();

          channel.setRemoteWindowSize(rws);
          channel.setRemotePacketSize(rps);
          channel.open_confirmation=true;
          channel.setRecipient(r);
          break;
	case SSH_MSG_CHANNEL_OPEN_FAILURE:
          buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel==null){
	    //break;
	  }
	  int reason_code=buf.getInt(); 
	  //foo=buf.getString();  // additional textual information
	  //foo=buf.getString();  // language tag 
          channel.setExitStatus(reason_code);
          channel.close=true;
	  channel.eof_remote=true;
	  channel.setRecipient(0);
	  break;
	case SSH_MSG_CHANNEL_REQUEST:
          buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  foo=buf.getString(); 
          boolean reply=(buf.getByte()!=0);
	  channel=Channel.getChannel(i, this);
	  if(channel!=null){
	    byte reply_type=(byte)SSH_MSG_CHANNEL_FAILURE;
	    if((Util.byte2str(foo)).equals("exit-status")){
	      i=buf.getInt();             // exit-status
	      channel.setExitStatus(i);
	      reply_type=(byte)SSH_MSG_CHANNEL_SUCCESS;
	    }
	    if(reply){
	      packet.reset();
	      buf.putByte(reply_type);
	      buf.putInt(channel.getRecipient());
	      write(packet);
	    }
	  }
	  else{
	  }
	  break;
	case SSH_MSG_CHANNEL_OPEN:
          buf.getInt(); 
	  buf.getShort(); 
	  foo=buf.getString(); 
	  String ctyp=Util.byte2str(foo);
          if(!"forwarded-tcpip".equals(ctyp) &&
	     !("x11".equals(ctyp) && x11_forwarding) &&
	     !("auth-agent@openssh.com".equals(ctyp) && agent_forwarding)){
            //System.err.println("Session.run: CHANNEL OPEN "+ctyp); 
	    //throw new IOException("Session.run: CHANNEL OPEN "+ctyp);
	    packet.reset();
	    buf.putByte((byte)SSH_MSG_CHANNEL_OPEN_FAILURE);
	    buf.putInt(buf.getInt());
 	    buf.putInt(Channel.SSH_OPEN_ADMINISTRATIVELY_PROHIBITED);
	    buf.putString(Util.empty);
	    buf.putString(Util.empty);
	    write(packet);
	  }
	  else{
	    channel=Channel.getChannel(ctyp);
	    addChannel(channel);
	    channel.getData(buf);
	    channel.init();

	    Thread tmp=new Thread(channel);
	    tmp.setName("Channel "+ctyp+" "+host);
            if(daemon_thread){
              tmp.setDaemon(daemon_thread);
            }
	    tmp.start();
	    break;
	  }
	case SSH_MSG_CHANNEL_SUCCESS:
          buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel==null){
	    break;
	  }
	  channel.reply=1;
	  break;
	case SSH_MSG_CHANNEL_FAILURE:
	  buf.getInt(); 
	  buf.getShort(); 
	  i=buf.getInt(); 
	  channel=Channel.getChannel(i, this);
	  if(channel==null){
	    break;
	  }
	  channel.reply=0;
	  break;
	case SSH_MSG_GLOBAL_REQUEST:
	  buf.getInt(); 
	  buf.getShort(); 
	  foo=buf.getString();       // request name
	  reply=(buf.getByte()!=0);
	  if(reply){
	    packet.reset();
	    buf.putByte((byte)SSH_MSG_REQUEST_FAILURE);
	    write(packet);
	  }
	  break;
	case SSH_MSG_REQUEST_FAILURE:
	case SSH_MSG_REQUEST_SUCCESS:
          Thread t=grr.getThread();
          if(t!=null){
            grr.setReply(msgType==SSH_MSG_REQUEST_SUCCESS? 1 : 0);
            t.interrupt();
          }
	  break;
	default:
          //System.err.println("Session.run: unsupported type "+msgType); 
	  throw new IOException("Unknown SSH message type "+msgType);
	}
      }
    }
    catch(Exception e){
      in_kex=false;
      if(JSch.getLogger().isEnabled(Logger.INFO)){
        JSch.getLogger().log(Logger.INFO,
                             "Caught an exception, leaving main loop due to " + e.getMessage());
      }
      //System.err.println("# Session.run");
      //e.printStackTrace();
    }
    try{
      disconnect();
    }
    catch(NullPointerException e){
      //System.err.println("@1");
      //e.printStackTrace();
    }
    catch(Exception e){
      //System.err.println("@2");
      //e.printStackTrace();
    }
    isConnected=false;
  }

  public void disconnect(){
    if(!isConnected) return;
    //System.err.println(this+": disconnect");
    //Thread.dumpStack();
    if(JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO,
                           "Disconnecting from "+host+" port "+port);
    }
    /*
    for(int i=0; i<Channel.pool.size(); i++){
      try{
        Channel c=((Channel)(Channel.pool.elementAt(i)));
	if(c.session==this) c.eof();
      }
      catch(Exception e){
      }
    } 
    */

    Channel.disconnect(this);

    isConnected=false;

    PortWatcher.delPort(this);
    ChannelForwardedTCPIP.delPort(this);
    ChannelX11.removeFakedCookie(this);

    synchronized(lock){
      if(connectThread!=null){
        Thread.yield();
        connectThread.interrupt();
        connectThread=null;
      }
    }
    thread=null;
    try{
      if(io!=null){
	if(io.in!=null) io.in.close();
	if(io.out!=null) io.out.close();
	if(io.out_ext!=null) io.out_ext.close();
      }
      if(proxy==null){
        if(socket!=null)
	  socket.close();
      }
      else{
	synchronized(proxy){
	  proxy.close();	  
	}
	proxy=null;
      }
    }
    catch(Exception e){
//      e.printStackTrace();
    }
    io=null;
    socket=null;
//    synchronized(jsch.pool){
//      jsch.pool.removeElement(this);
//    }

    jsch.removeSession(this);

    //System.gc();
  }

  public int setPortForwardingL(int lport, String host, int rport) throws JSchException{
    return setPortForwardingL("127.0.0.1", lport, host, rport);
  }
  public int setPortForwardingL(String boundaddress, int lport, String host, int rport) throws JSchException{
    return setPortForwardingL(boundaddress, lport, host, rport, null);
  }
  public int setPortForwardingL(String boundaddress, int lport, String host, int rport, ServerSocketFactory ssf) throws JSchException{
    PortWatcher pw=PortWatcher.addPort(this, boundaddress, lport, host, rport, ssf);
    Thread tmp=new Thread(pw);
    tmp.setName("PortWatcher Thread for "+host);
    if(daemon_thread){
      tmp.setDaemon(daemon_thread);
    }
    tmp.start();
    return pw.lport;
  }
  public void delPortForwardingL(int lport) throws JSchException{
    delPortForwardingL("127.0.0.1", lport);
  }
  public void delPortForwardingL(String boundaddress, int lport) throws JSchException{
    PortWatcher.delPort(this, boundaddress, lport);
  }
  public String[] getPortForwardingL() throws JSchException{
    return PortWatcher.getPortForwarding(this);
  }

  public void setPortForwardingR(int rport, String host, int lport) throws JSchException{
    setPortForwardingR(null, rport, host, lport, (SocketFactory)null);
  }
  public void setPortForwardingR(String bind_address, int rport, String host, int lport) throws JSchException{
    setPortForwardingR(bind_address, rport, host, lport, (SocketFactory)null);
  }
  public void setPortForwardingR(int rport, String host, int lport, SocketFactory sf) throws JSchException{
    setPortForwardingR(null, rport, host, lport, sf);
  }
  public void setPortForwardingR(String bind_address, int rport, String host, int lport, SocketFactory sf) throws JSchException{
    ChannelForwardedTCPIP.addPort(this, bind_address, rport, host, lport, sf);
    setPortForwarding(bind_address, rport);
  }

  public void setPortForwardingR(int rport, String daemon) throws JSchException{
    setPortForwardingR(null, rport, daemon, null);
  }
  public void setPortForwardingR(int rport, String daemon, Object[] arg) throws JSchException{
    setPortForwardingR(null, rport, daemon, arg);
  }
  public void setPortForwardingR(String bind_address, int rport, String daemon, Object[] arg) throws JSchException{
    ChannelForwardedTCPIP.addPort(this, bind_address, rport, daemon, arg);
    setPortForwarding(bind_address, rport);
  }

  private class GlobalRequestReply{
    private Thread thread=null;
    private int reply=-1;
    void setThread(Thread thread){
      this.thread=thread;
      this.reply=-1;
    }
    Thread getThread(){ return thread; }
    void setReply(int reply){ this.reply=reply; }
    int getReply(){ return this.reply; }
  }
  private GlobalRequestReply grr=new GlobalRequestReply();
  private void setPortForwarding(String bind_address, int rport) throws JSchException{
    synchronized(grr){
    Buffer buf=new Buffer(100); // ??
    Packet packet=new Packet(buf);

    String address_to_bind=ChannelForwardedTCPIP.normalize(bind_address);

    grr.setThread(Thread.currentThread());

    try{
      // byte SSH_MSG_GLOBAL_REQUEST 80
      // string "tcpip-forward"
      // boolean want_reply
      // string  address_to_bind
      // uint32  port number to bind
      packet.reset();
      buf.putByte((byte) SSH_MSG_GLOBAL_REQUEST);
      buf.putString(Util.str2byte("tcpip-forward"));
      buf.putByte((byte)1);
      buf.putString(Util.str2byte(address_to_bind));
      buf.putInt(rport);
      write(packet);
    }
    catch(Exception e){
      grr.setThread(null);
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }

    int count = 0;
    int reply = grr.getReply();
    while(count < 10 && reply == -1){
      try{ Thread.sleep(1000); }
      catch(Exception e){
      }
      count++; 
      reply = grr.getReply();
    }
    grr.setThread(null);
    if(reply != 1){
      throw new JSchException("remote port forwarding failed for listen port "+rport);
    }
    }
  }
  public void delPortForwardingR(int rport) throws JSchException{
    ChannelForwardedTCPIP.delPort(this, rport);
  }

  private void initDeflater(String method) throws JSchException{
    if(method.equals("none")){
      deflater=null;
      return;
    }
    String foo=getConfig(method);
    if(foo!=null){
      if(method.equals("zlib") ||
         (isAuthed && method.equals("zlib@openssh.com"))){
        try{
          Class c=Class.forName(foo);
          deflater=(Compression)(c.newInstance());
          int level=6;
          try{ level=Integer.parseInt(getConfig("compression_level"));}
          catch(Exception ee){ }
          deflater.init(Compression.DEFLATER, level);
        }
        catch(Exception ee){
          throw new JSchException(ee.toString(), ee);
          //System.err.println(foo+" isn't accessible.");
        }
      }
    }
  }
  private void initInflater(String method) throws JSchException{
    if(method.equals("none")){
      inflater=null;
      return;
    }
    String foo=getConfig(method);
    if(foo!=null){
      if(method.equals("zlib") ||
         (isAuthed && method.equals("zlib@openssh.com"))){
        try{
          Class c=Class.forName(foo);
          inflater=(Compression)(c.newInstance());
          inflater.init(Compression.INFLATER, 0);
        }
        catch(Exception ee){
          throw new JSchException(ee.toString(), ee);
	    //System.err.println(foo+" isn't accessible.");
        }
      }
    }
  }

  void addChannel(Channel channel){
    channel.setSession(this);
  }

  public void setProxy(Proxy proxy){ this.proxy=proxy; }
  public void setHost(String host){ this.host=host; }
  public void setPort(int port){ this.port=port; }
  void setUserName(String username){ this.username=username; }
  public void setUserInfo(UserInfo userinfo){ this.userinfo=userinfo; }
  public UserInfo getUserInfo(){ return userinfo; }
  public void setInputStream(InputStream in){ this.in=in; }
  public void setOutputStream(OutputStream out){ this.out=out; }
  public void setX11Host(String host){ ChannelX11.setHost(host); }
  public void setX11Port(int port){ ChannelX11.setPort(port); }
  public void setX11Cookie(String cookie){ ChannelX11.setCookie(cookie); }
  public void setPassword(String password){
    if(password!=null)
      this.password=Util.str2byte(password);
  }
  public void setPassword(byte[] password){ 
    if(password!=null){
      this.password=new byte[password.length];
      System.arraycopy(password, 0, this.password, 0, password.length);
    }
  }

  public void setConfig(java.util.Properties newconf){
    setConfig((java.util.Hashtable)newconf);
  }
 
  public void setConfig(java.util.Hashtable newconf){
    synchronized(lock){
      if(config==null) 
        config=new java.util.Hashtable();
      for(java.util.Enumeration e=newconf.keys() ; e.hasMoreElements() ;) {
        String key=(String)(e.nextElement());
        config.put(key, (String)(newconf.get(key)));
      }
    }
  }

  public void setConfig(String key, String value){
    synchronized(lock){ 
      if(config==null){
        config=new java.util.Hashtable();
      }
      config.put(key, value);
    }
  }

  public String getConfig(String key){
    Object foo=null;
    if(config!=null){
      foo=config.get(key);
      if(foo instanceof String) return (String)foo;
    }
    foo=jsch.getConfig(key);
    if(foo instanceof String) return (String)foo;
    return null;
  }

  public void setSocketFactory(SocketFactory sfactory){ 
    socket_factory=sfactory;
  }
  public boolean isConnected(){ return isConnected; }
  public int getTimeout(){ return timeout; }
  public void setTimeout(int timeout) throws JSchException {
    if(socket==null){
      if(timeout<0){
        throw new JSchException("invalid timeout value");
      }
      this.timeout=timeout;
      return;
    }
    try{
      socket.setSoTimeout(timeout);
      this.timeout=timeout;
    }
    catch(Exception e){
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }
  }
  public String getServerVersion(){
    return Util.byte2str(V_S);
  }
  public String getClientVersion(){
    return Util.byte2str(V_C);
  }
  public void setClientVersion(String cv){
    V_C=Util.str2byte(cv);
  }

  public void sendIgnore() throws Exception{
    Buffer buf=new Buffer();
    Packet packet=new Packet(buf);
    packet.reset();
    buf.putByte((byte)SSH_MSG_IGNORE);
    write(packet);
  }

  private static final byte[] keepalivemsg=Util.str2byte("keepalive@jcraft.com");
  public void sendKeepAliveMsg() throws Exception{
    Buffer buf=new Buffer();
    Packet packet=new Packet(buf);
    packet.reset();
    buf.putByte((byte)SSH_MSG_GLOBAL_REQUEST);
    buf.putString(keepalivemsg);
    buf.putByte((byte)1);
    write(packet);
  }
  
  private HostKey hostkey=null;
  public HostKey getHostKey(){ return hostkey; }
  public String getHost(){return host;}
  public String getUserName(){return username;}
  public int getPort(){return port;}
  public void setHostKeyAlias(String hostKeyAlias){
    this.hostKeyAlias=hostKeyAlias;
  }
  public String getHostKeyAlias(){
    return hostKeyAlias;
  }

  public void setServerAliveInterval(int interval) throws JSchException {
    setTimeout(interval);
    this.serverAliveInterval=interval;
  }
  public void setServerAliveCountMax(int count){
    this.serverAliveCountMax=count;
  }

  public int getServerAliveInterval(){
    return this.serverAliveInterval;
  }
  public int getServerAliveCountMax(){
    return this.serverAliveCountMax;
  }

  public void setDaemonThread(boolean enable){
    this.daemon_thread=enable;
  }

  private String[] checkCiphers(String ciphers){
    if(ciphers==null || ciphers.length()==0)
      return null;

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO, 
                           "CheckCiphers: "+ciphers);
    }

    java.util.Vector result=new java.util.Vector();
    String[] _ciphers=Util.split(ciphers, ",");
    for(int i=0; i<_ciphers.length; i++){
      if(!checkCipher(getConfig(_ciphers[i]))){
        result.addElement(_ciphers[i]);
      }
    }
    if(result.size()==0)
      return null;
    String[] foo=new String[result.size()];
    System.arraycopy(result.toArray(), 0, foo, 0, result.size());

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      for(int i=0; i<foo.length; i++){
        JSch.getLogger().log(Logger.INFO, 
                             foo[i]+" is not available.");
      }
    }

    return foo;
  }

  static boolean checkCipher(String cipher){
    try{
      Class c=Class.forName(cipher);
      Cipher _c=(Cipher)(c.newInstance());
      _c.init(Cipher.ENCRYPT_MODE,
              new byte[_c.getBlockSize()],
              new byte[_c.getIVSize()]);
      return true;
    }
    catch(Exception e){
      return false;
    }
  }

  private String[] checkKexes(String kexes){
    if(kexes==null || kexes.length()==0)
      return null;

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      JSch.getLogger().log(Logger.INFO, 
                           "CheckKexes: "+kexes);
    }

    java.util.Vector result=new java.util.Vector();
    String[] _kexes=Util.split(kexes, ",");
    for(int i=0; i<_kexes.length; i++){
      if(!checkKex(this, getConfig(_kexes[i]))){
        result.addElement(_kexes[i]);
      }
    }
    if(result.size()==0)
      return null;
    String[] foo=new String[result.size()];
    System.arraycopy(result.toArray(), 0, foo, 0, result.size());

    if(JSch.getLogger().isEnabled(Logger.INFO)){
      for(int i=0; i<foo.length; i++){
        JSch.getLogger().log(Logger.INFO, 
                             foo[i]+" is not available.");
      }
    }

    return foo;
  }

  static boolean checkKex(Session s, String kex){
    try{
      Class c=Class.forName(kex);
      KeyExchange _c=(KeyExchange)(c.newInstance());
      _c.init(s ,null, null, null, null);
      return true;
    }
    catch(Exception e){ return false; }
  }
}
