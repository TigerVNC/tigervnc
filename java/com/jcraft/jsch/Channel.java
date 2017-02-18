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

import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;


public abstract class Channel implements Runnable{

  static final int SSH_MSG_CHANNEL_OPEN_CONFIRMATION=      91;
  static final int SSH_MSG_CHANNEL_OPEN_FAILURE=           92;
  static final int SSH_MSG_CHANNEL_WINDOW_ADJUST=          93;

  static final int SSH_OPEN_ADMINISTRATIVELY_PROHIBITED=    1;
  static final int SSH_OPEN_CONNECT_FAILED=                 2;
  static final int SSH_OPEN_UNKNOWN_CHANNEL_TYPE=           3;
  static final int SSH_OPEN_RESOURCE_SHORTAGE=              4;

  static int index=0; 
  private static java.util.Vector pool=new java.util.Vector();
  static Channel getChannel(String type){
    if(type.equals("session")){
      return new ChannelSession();
    }
    if(type.equals("shell")){
      return new ChannelShell();
    }
    if(type.equals("exec")){
      return new ChannelExec();
    }
    if(type.equals("x11")){
      return new ChannelX11();
    }
    if(type.equals("auth-agent@openssh.com")){
      return new ChannelAgentForwarding();
    }
    if(type.equals("direct-tcpip")){
      return new ChannelDirectTCPIP();
    }
    if(type.equals("forwarded-tcpip")){
      return new ChannelForwardedTCPIP();
    }
    if(type.equals("sftp")){
      return new ChannelSftp();
    }
    if(type.equals("subsystem")){
      return new ChannelSubsystem();
    }
    return null;
  }
  static Channel getChannel(int id, Session session){
    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
        Channel c=(Channel)(pool.elementAt(i));
        if(c.id==id && c.session==session) return c;
      }
    }
    return null;
  }
  static void del(Channel c){
    synchronized(pool){
      pool.removeElement(c);
    }
  }

  int id;
  volatile int recipient=-1;
  protected byte[] type=Util.str2byte("foo");
  volatile int lwsize_max=0x100000;
  volatile int lwsize=lwsize_max;     // local initial window size
  volatile int lmpsize=0x4000;     // local maximum packet size

  volatile long rwsize=0;         // remote initial window size
  volatile int rmpsize=0;        // remote maximum packet size

  IO io=null;    
  Thread thread=null;

  volatile boolean eof_local=false;
  volatile boolean eof_remote=false;

  volatile boolean close=false;
  volatile boolean connected=false;
  volatile boolean open_confirmation=false;

  volatile int exitstatus=-1;

  volatile int reply=0; 
  volatile int connectTimeout=0;

  private Session session;

  int notifyme=0; 

  Channel(){
    synchronized(pool){
      id=index++;
      pool.addElement(this);
    }
  }
  synchronized void setRecipient(int foo){
    this.recipient=foo;
    if(notifyme>0)
      notifyAll();
  }
  int getRecipient(){
    return recipient;
  }

  void init() throws JSchException {
  }

  public void connect() throws JSchException{
    connect(0);
  }

  public void connect(int connectTimeout) throws JSchException{
    this.connectTimeout=connectTimeout;
    try{
      sendChannelOpen();
      start();
    }
    catch(Exception e){
      connected=false;
      disconnect();
      if(e instanceof JSchException) 
        throw (JSchException)e;
      throw new JSchException(e.toString(), e);
    }
  }

  public void setXForwarding(boolean foo){
  }

  public void start() throws JSchException{}

  public boolean isEOF() {return eof_remote;}

  void getData(Buffer buf){
    setRecipient(buf.getInt());
    setRemoteWindowSize(buf.getUInt());
    setRemotePacketSize(buf.getInt());
  }

  public void setInputStream(InputStream in){
    io.setInputStream(in, false);
  }
  public void setInputStream(InputStream in, boolean dontclose){
    io.setInputStream(in, dontclose);
  }
  public void setOutputStream(OutputStream out){
    io.setOutputStream(out, false);
  }
  public void setOutputStream(OutputStream out, boolean dontclose){
    io.setOutputStream(out, dontclose);
  }
  public void setExtOutputStream(OutputStream out){
    io.setExtOutputStream(out, false);
  }
  public void setExtOutputStream(OutputStream out, boolean dontclose){
    io.setExtOutputStream(out, dontclose);
  }
  public InputStream getInputStream() throws IOException {
    int max_input_buffer_size = 32*1024;
    try {
      max_input_buffer_size =
        Integer.parseInt(getSession().getConfig("max_input_buffer_size"));
    }
    catch(Exception e){}
    PipedInputStream in =
      new MyPipedInputStream(
                             32*1024,  // this value should be customizable.
                             max_input_buffer_size
                             );
    boolean resizable = 32*1024<max_input_buffer_size;
    io.setOutputStream(new PassiveOutputStream(in, resizable), false);
    return in;
  }
  public InputStream getExtInputStream() throws IOException {
    int max_input_buffer_size = 32*1024;
    try {
      max_input_buffer_size =
        Integer.parseInt(getSession().getConfig("max_input_buffer_size"));
    }
    catch(Exception e){}
    PipedInputStream in =
      new MyPipedInputStream(
                             32*1024,  // this value should be customizable.
                             max_input_buffer_size
                             );
    boolean resizable = 32*1024<max_input_buffer_size;
    io.setExtOutputStream(new PassiveOutputStream(in, resizable), false);
    return in;
  }
  public OutputStream getOutputStream() throws IOException {

    final Channel channel=this;
    OutputStream out=new OutputStream(){
        private int dataLen=0;
        private Buffer buffer=null;
        private Packet packet=null;
        private boolean closed=false;
        private synchronized void init() throws java.io.IOException{
          buffer=new Buffer(rmpsize);
          packet=new Packet(buffer);

          byte[] _buf=buffer.buffer;
          if(_buf.length-(14+0)-Session.buffer_margin<=0){
            buffer=null;
            packet=null;
            throw new IOException("failed to initialize the channel.");
          }

        }
        byte[] b=new byte[1];
        public void write(int w) throws java.io.IOException{
          b[0]=(byte)w;
          write(b, 0, 1);
        }
        public void write(byte[] buf, int s, int l) throws java.io.IOException{
          if(packet==null){
            init();
          }

          if(closed){
            throw new java.io.IOException("Already closed");
          }

          byte[] _buf=buffer.buffer;
          int _bufl=_buf.length;
          while(l>0){
            int _l=l;
            if(l>_bufl-(14+dataLen)-Session.buffer_margin){
              _l=_bufl-(14+dataLen)-Session.buffer_margin;
            }

            if(_l<=0){
              flush();
              continue;
            }

            System.arraycopy(buf, s, _buf, 14+dataLen, _l);
            dataLen+=_l;
            s+=_l;
            l-=_l;
          }
        }

        public void flush() throws java.io.IOException{
          if(closed){
            throw new java.io.IOException("Already closed");
          }
          if(dataLen==0)
            return;
          packet.reset();
          buffer.putByte((byte)Session.SSH_MSG_CHANNEL_DATA);
          buffer.putInt(recipient);
          buffer.putInt(dataLen);
          buffer.skip(dataLen);
          try{
            int foo=dataLen;
            dataLen=0;
            synchronized(channel){
              if(!channel.close)
                getSession().write(packet, channel, foo);
            }
          }
          catch(Exception e){
            close();
            throw new java.io.IOException(e.toString());
          }

        }
        public void close() throws java.io.IOException{
          if(packet==null){
            try{
              init();
            }
            catch(java.io.IOException e){
              // close should be finished silently.
              return;
            }
          }
          if(closed){
            return;
          }
          if(dataLen>0){
            flush();
          }
          channel.eof();
          closed=true;
        }
      };
    return out;
  }

  class MyPipedInputStream extends PipedInputStream{
    private int BUFFER_SIZE = 1024;
    private int max_buffer_size = BUFFER_SIZE;
    MyPipedInputStream() throws IOException{ super(); }
    MyPipedInputStream(int size) throws IOException{
      super();
      buffer=new byte[size];
      BUFFER_SIZE = size;
      max_buffer_size = size;
    }
    MyPipedInputStream(int size, int max_buffer_size) throws IOException{
      this(size);
      this.max_buffer_size = max_buffer_size;
    }
    MyPipedInputStream(PipedOutputStream out) throws IOException{ super(out); }
    MyPipedInputStream(PipedOutputStream out, int size) throws IOException{
      super(out);
      buffer=new byte[size];
      BUFFER_SIZE=size;
    }

    /*
     * TODO: We should have our own Piped[I/O]Stream implementation.
     * Before accepting data, JDK's PipedInputStream will check the existence of
     * reader thread, and if it is not alive, the stream will be closed.
     * That behavior may cause the problem if multiple threads make access to it.
     */
    public synchronized void updateReadSide() throws IOException {
      if(available() != 0){ // not empty
        return;
      }
      in = 0;
      out = 0;
      buffer[in++] = 0;
      read();
    }

    private int freeSpace(){
      int size = 0;
      if(out < in) {
        size = buffer.length-in;
      }
      else if(in < out){
        if(in == -1) size = buffer.length;
        else size = out - in;
      }
      return size;
    } 
    synchronized void checkSpace(int len) throws IOException {
      int size = freeSpace();
      if(size<len){
        int datasize=buffer.length-size;
        int foo = buffer.length;
        while((foo - datasize) < len){
          foo*=2;
        }

        if(foo > max_buffer_size){
          foo = max_buffer_size;
        }
        if((foo - datasize) < len) return;

        byte[] tmp = new byte[foo];
        if(out < in) {
          System.arraycopy(buffer, 0, tmp, 0, buffer.length);
        }
        else if(in < out){
          if(in == -1) {
          }
          else {
            System.arraycopy(buffer, 0, tmp, 0, in);
            System.arraycopy(buffer, out, 
                             tmp, tmp.length-(buffer.length-out),
                             (buffer.length-out));
            out = tmp.length-(buffer.length-out);
          }
        }
        else if(in == out){
          System.arraycopy(buffer, 0, tmp, 0, buffer.length);
          in=buffer.length;
        }
        buffer=tmp;
      }
      else if(buffer.length == size && size > BUFFER_SIZE) { 
        int  i = size/2;
        if(i<BUFFER_SIZE) i = BUFFER_SIZE;
        byte[] tmp = new byte[i];
        buffer=tmp;
      }
    }
  }
  void setLocalWindowSizeMax(int foo){ this.lwsize_max=foo; }
  void setLocalWindowSize(int foo){ this.lwsize=foo; }
  void setLocalPacketSize(int foo){ this.lmpsize=foo; }
  synchronized void setRemoteWindowSize(long foo){ this.rwsize=foo; }
  synchronized void addRemoteWindowSize(long foo){ 
    this.rwsize+=foo; 
    if(notifyme>0)
      notifyAll();
  }
  void setRemotePacketSize(int foo){ this.rmpsize=foo; }

  public void run(){
  }

  void write(byte[] foo) throws IOException {
    write(foo, 0, foo.length);
  }
  void write(byte[] foo, int s, int l) throws IOException {
    try{
      io.put(foo, s, l);
    }catch(NullPointerException e){}
  }
  void write_ext(byte[] foo, int s, int l) throws IOException {
    try{
      io.put_ext(foo, s, l);
    }catch(NullPointerException e){}
  }

  void eof_remote(){
    eof_remote=true;
    try{
      io.out_close();
    }
    catch(NullPointerException e){}
  }

  void eof(){
    if(eof_local)return;
    eof_local=true;

    int i = getRecipient();
    if(i == -1) return;

    try{
      Buffer buf=new Buffer(100);
      Packet packet=new Packet(buf);
      packet.reset();
      buf.putByte((byte)Session.SSH_MSG_CHANNEL_EOF);
      buf.putInt(i);
      synchronized(this){
        if(!close)
          getSession().write(packet);
      }
    }
    catch(Exception e){
      //System.err.println("Channel.eof");
      //e.printStackTrace();
    }
    /*
    if(!isConnected()){ disconnect(); }
    */
  }

  /*
  http://www1.ietf.org/internet-drafts/draft-ietf-secsh-connect-24.txt

5.3  Closing a Channel
  When a party will no longer send more data to a channel, it SHOULD
   send SSH_MSG_CHANNEL_EOF.

            byte      SSH_MSG_CHANNEL_EOF
            uint32    recipient_channel

  No explicit response is sent to this message.  However, the
   application may send EOF to whatever is at the other end of the
  channel.  Note that the channel remains open after this message, and
   more data may still be sent in the other direction.  This message
   does not consume window space and can be sent even if no window space
   is available.

     When either party wishes to terminate the channel, it sends
     SSH_MSG_CHANNEL_CLOSE.  Upon receiving this message, a party MUST
   send back a SSH_MSG_CHANNEL_CLOSE unless it has already sent this
   message for the channel.  The channel is considered closed for a
     party when it has both sent and received SSH_MSG_CHANNEL_CLOSE, and
   the party may then reuse the channel number.  A party MAY send
   SSH_MSG_CHANNEL_CLOSE without having sent or received
   SSH_MSG_CHANNEL_EOF.

            byte      SSH_MSG_CHANNEL_CLOSE
            uint32    recipient_channel

   This message does not consume window space and can be sent even if no
   window space is available.

   It is recommended that any data sent before this message is delivered
     to the actual destination, if possible.
  */

  void close(){
    if(close)return;
    close=true;
    eof_local=eof_remote=true;

    int i = getRecipient();
    if(i == -1) return;

    try{
      Buffer buf=new Buffer(100);
      Packet packet=new Packet(buf);
      packet.reset();
      buf.putByte((byte)Session.SSH_MSG_CHANNEL_CLOSE);
      buf.putInt(i);
      synchronized(this){
        getSession().write(packet);
      }
    }
    catch(Exception e){
      //e.printStackTrace();
    }
  }
  public boolean isClosed(){
    return close;
  }
  static void disconnect(Session session){
    Channel[] channels=null;
    int count=0;
    synchronized(pool){
      channels=new Channel[pool.size()];
      for(int i=0; i<pool.size(); i++){
	try{
	  Channel c=((Channel)(pool.elementAt(i)));
	  if(c.session==session){
	    channels[count++]=c;
	  }
	}
	catch(Exception e){
	}
      } 
    }
    for(int i=0; i<count; i++){
      channels[i].disconnect();
    }
  }

  public void disconnect(){
    //System.err.println(this+":disconnect "+io+" "+connected);
    //Thread.dumpStack();

    try{

      synchronized(this){
        if(!connected){
          return;
        }
        connected=false;
      }

      close();

      eof_remote=eof_local=true;

      thread=null;

      try{
        if(io!=null){
          io.close();
        }
      }
      catch(Exception e){
        //e.printStackTrace();
      }
      // io=null;
    }
    finally{
      Channel.del(this);
    }
  }

  public boolean isConnected(){
    Session _session=this.session;
    if(_session!=null){
      return _session.isConnected() && connected;
    }
    return false;
  }

  public void sendSignal(String signal) throws Exception {
    RequestSignal request=new RequestSignal();
    request.setSignal(signal);
    request.request(getSession(), this);
  }

//  public String toString(){
//      return "Channel: type="+new String(type)+",id="+id+",recipient="+recipient+",window_size="+window_size+",packet_size="+packet_size;
//  }

/*
  class OutputThread extends Thread{
    Channel c;
    OutputThread(Channel c){ this.c=c;}
    public void run(){c.output_thread();}
  }
*/

  class PassiveInputStream extends MyPipedInputStream{
    PipedOutputStream out;
    PassiveInputStream(PipedOutputStream out, int size) throws IOException{
      super(out, size);
      this.out=out;
    }
    PassiveInputStream(PipedOutputStream out) throws IOException{
      super(out);
      this.out=out;
    }
    public void close() throws IOException{
      if(out!=null){
        this.out.close();
      }
      out=null;
    }
  }
  class PassiveOutputStream extends PipedOutputStream{
    private MyPipedInputStream _sink=null;
    PassiveOutputStream(PipedInputStream in,
                        boolean resizable_buffer) throws IOException{
      super(in);
      if(resizable_buffer && (in instanceof MyPipedInputStream)) {
        this._sink=(MyPipedInputStream)in;
      }
    }
    public void write(int b) throws IOException {
      if(_sink != null) {
        _sink.checkSpace(1);
      }
      super.write(b);
    }
    public void write(byte[] b, int off, int len) throws IOException {
      if(_sink != null) {
        _sink.checkSpace(len);
      }
      super.write(b, off, len); 
    }
  }

  void setExitStatus(int status){ exitstatus=status; }
  public int getExitStatus(){ return exitstatus; }

  void setSession(Session session){
    this.session=session;
  }

  public Session getSession() throws JSchException{ 
    Session _session=session;
    if(_session==null){
      throw new JSchException("session is not available");
    }
    return _session;
  }
  public int getId(){ return id; }

  protected void sendOpenConfirmation() throws Exception{
    Buffer buf=new Buffer(100);
    Packet packet=new Packet(buf);
    packet.reset();
    buf.putByte((byte)SSH_MSG_CHANNEL_OPEN_CONFIRMATION);
    buf.putInt(getRecipient());
    buf.putInt(id);
    buf.putInt(lwsize);
    buf.putInt(lmpsize);
    getSession().write(packet);
  }

  protected void sendOpenFailure(int reasoncode){
    try{
      Buffer buf=new Buffer(100);
      Packet packet=new Packet(buf);
      packet.reset();
      buf.putByte((byte)SSH_MSG_CHANNEL_OPEN_FAILURE);
      buf.putInt(getRecipient());
      buf.putInt(reasoncode);
      buf.putString(Util.str2byte("open failed"));
      buf.putString(Util.empty);
      getSession().write(packet);
    }
    catch(Exception e){
    }
  }

  protected Packet genChannelOpenPacket(){
    Buffer buf=new Buffer(100);
    Packet packet=new Packet(buf);
    // byte   SSH_MSG_CHANNEL_OPEN(90)
    // string channel type         //
    // uint32 sender channel       // 0
    // uint32 initial window size  // 0x100000(65536)
    // uint32 maxmum packet size   // 0x4000(16384)
    packet.reset();
    buf.putByte((byte)90);
    buf.putString(this.type);
    buf.putInt(this.id);
    buf.putInt(this.lwsize);
    buf.putInt(this.lmpsize);
    return packet;
  }

  protected void sendChannelOpen() throws Exception {
    Session _session=getSession();
    if(!_session.isConnected()){
      throw new JSchException("session is down");
    }

    Packet packet = genChannelOpenPacket();
    _session.write(packet);

    int retry=2000;
    long start=System.currentTimeMillis();
    long timeout=connectTimeout;
    if(timeout!=0L) retry = 1;
    synchronized(this){
      while(this.getRecipient()==-1 &&
            _session.isConnected() &&
             retry>0){
        if(timeout>0L){
          if((System.currentTimeMillis()-start)>timeout){
            retry=0;
            continue;
          }
        }
        try{
          long t = timeout==0L ? 10L : timeout;
          this.notifyme=1;
          wait(t);
        }
        catch(java.lang.InterruptedException e){
        }
        finally{
          this.notifyme=0;
        }
        retry--;
      }
    }
    if(!_session.isConnected()){
      throw new JSchException("session is down");
    }
    if(this.getRecipient()==-1){  // timeout
      throw new JSchException("channel is not opened.");
    }
    if(this.open_confirmation==false){  // SSH_MSG_CHANNEL_OPEN_FAILURE
      throw new JSchException("channel is not opened.");
    }
    connected=true;
  }
}
