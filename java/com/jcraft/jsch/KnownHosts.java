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

public
class KnownHosts implements HostKeyRepository{
  private static final String _known_hosts="known_hosts";

  /*
  static final int SSHDSS=0;
  static final int SSHRSA=1;
  static final int UNKNOWN=2;
  */

  private JSch jsch=null;
  private String known_hosts=null;
  private java.util.Vector pool=null;

  private MAC hmacsha1=null;

  KnownHosts(JSch jsch){
    super();
    this.jsch=jsch;
    pool=new java.util.Vector();
  }

  void setKnownHosts(String foo) throws JSchException{
    try{
      known_hosts=foo;
      FileInputStream fis=new FileInputStream(foo);
      setKnownHosts(fis);
    }
    catch(FileNotFoundException e){
    } 
  }
  void setKnownHosts(InputStream foo) throws JSchException{
    pool.removeAllElements();
    StringBuffer sb=new StringBuffer();
    byte i;
    int j;
    boolean error=false;
    try{
      InputStream fis=foo;
      String host;
      String key=null;
      int type;
      byte[] buf=new byte[1024];
      int bufl=0;
loop:
      while(true){
	bufl=0;
        while(true){
          j=fis.read();
          if(j==-1){
            if(bufl==0){ break loop; }
            else{ break; }
          }
	  if(j==0x0d){ continue; }
	  if(j==0x0a){ break; }
          if(buf.length<=bufl){
            if(bufl>1024*10) break;   // too long...
            byte[] newbuf=new byte[buf.length*2];
            System.arraycopy(buf, 0, newbuf, 0, buf.length);
            buf=newbuf;
          }
          buf[bufl++]=(byte)j;
	}

	j=0;
        while(j<bufl){
          i=buf[j];
	  if(i==' '||i=='\t'){ j++; continue; }
	  if(i=='#'){
	    addInvalidLine(Util.byte2str(buf, 0, bufl));
	    continue loop;
	  }
	  break;
	}
	if(j>=bufl){ 
	  addInvalidLine(Util.byte2str(buf, 0, bufl));
	  continue loop; 
	}

        sb.setLength(0);
        while(j<bufl){
          i=buf[j++];
          if(i==0x20 || i=='\t'){ break; }
          sb.append((char)i);
	}
	host=sb.toString();
	if(j>=bufl || host.length()==0){
	  addInvalidLine(Util.byte2str(buf, 0, bufl));
	  continue loop; 
	}

        sb.setLength(0);
	type=-1;
        while(j<bufl){
          i=buf[j++];
          if(i==0x20 || i=='\t'){ break; }
          sb.append((char)i);
	}
	if(sb.toString().equals("ssh-dss")){ type=HostKey.SSHDSS; }
	else if(sb.toString().equals("ssh-rsa")){ type=HostKey.SSHRSA; }
	else { j=bufl; }
	if(j>=bufl){
	  addInvalidLine(Util.byte2str(buf, 0, bufl));
	  continue loop; 
	}

        sb.setLength(0);
        while(j<bufl){
          i=buf[j++];
          if(i==0x0d){ continue; }
          if(i==0x0a){ break; }
          sb.append((char)i);
	}
	key=sb.toString();
	if(key.length()==0){
	  addInvalidLine(Util.byte2str(buf, 0, bufl));
	  continue loop; 
	}

	//System.err.println(host);
	//System.err.println("|"+key+"|");

	HostKey hk = null;
        hk = new HashedHostKey(host, type, 
                               Util.fromBase64(Util.str2byte(key), 0, 
                                               key.length()));
	pool.addElement(hk);
      }
      fis.close();
      if(error){
	throw new JSchException("KnownHosts: invalid format");
      }
    }
    catch(Exception e){
      if(e instanceof JSchException)
	throw (JSchException)e;         
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }
  }
  private void addInvalidLine(String line) throws JSchException {
    HostKey hk = new HostKey(line, HostKey.UNKNOWN, null);
    pool.addElement(hk);
  }
  String getKnownHostsFile(){ return known_hosts; }
  public String getKnownHostsRepositoryID(){ return known_hosts; }

  public int check(String host, byte[] key){
    int result=NOT_INCLUDED;
    if(host==null){
      return result;
    }

    int type=getType(key);
    HostKey hk;

    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
        hk=(HostKey)(pool.elementAt(i));
        if(hk.isMatched(host) && hk.type==type){
          if(Util.array_equals(hk.key, key)){
            return OK;
          }
          else{
            result=CHANGED;
	  }
        }
      }
    }

    if(result==NOT_INCLUDED &&
       host.startsWith("[") &&
       host.indexOf("]:")>1
       ){
      return check(host.substring(1, host.indexOf("]:")), key);
    }

    return result;
  }
  public void add(HostKey hostkey, UserInfo userinfo){
    int type=hostkey.type;
    String host=hostkey.getHost();
    byte[] key=hostkey.key;

    HostKey hk=null;
    synchronized(pool){
      for(int i=0; i<pool.size(); i++){
        hk=(HostKey)(pool.elementAt(i));
        if(hk.isMatched(host) && hk.type==type){
/*
	  if(Util.array_equals(hk.key, key)){ return; }
	  if(hk.host.equals(host)){
	    hk.key=key;
	    return;
	  }
	  else{
	    hk.host=deleteSubString(hk.host, host);
	    break;
	  }
*/
        }
      }
    }

    hk=hostkey;

    pool.addElement(hk);

    String bar=getKnownHostsRepositoryID();
    if(bar!=null){
      boolean foo=true;
      File goo=new File(bar);
      if(!goo.exists()){
        foo=false;
        if(userinfo!=null){
          foo=userinfo.promptYesNo(bar+" does not exist.\n"+
                                   "Are you sure you want to create it?"
                                   );
          goo=goo.getParentFile();
          if(foo && goo!=null && !goo.exists()){
            foo=userinfo.promptYesNo("The parent directory "+goo+" does not exist.\n"+
                                     "Are you sure you want to create it?"
                                     );
            if(foo){
              if(!goo.mkdirs()){
                userinfo.showMessage(goo+" has not been created.");
                foo=false;
              }
              else{
                userinfo.showMessage(goo+" has been succesfully created.\nPlease check its access permission.");
              }
            }
          }
          if(goo==null)foo=false;
        }
      }
      if(foo){
        try{ 
          sync(bar); 
        }
        catch(Exception e){ System.err.println("sync known_hosts: "+e); }
      }
    }
  }

  public HostKey[] getHostKey(){
    return getHostKey(null, null);
  }
  public HostKey[] getHostKey(String host, String type){
    synchronized(pool){
      int count=0;
      for(int i=0; i<pool.size(); i++){
	HostKey hk=(HostKey)pool.elementAt(i);
	if(hk.type==HostKey.UNKNOWN) continue;
	if(host==null || 
	   (hk.isMatched(host) && 
	    (type==null || hk.getType().equals(type)))){
	  count++;
	}
      }
      if(count==0)return null;
      HostKey[] foo=new HostKey[count];
      int j=0;
      for(int i=0; i<pool.size(); i++){
	HostKey hk=(HostKey)pool.elementAt(i);
	if(hk.type==HostKey.UNKNOWN) continue;
	if(host==null || 
	   (hk.isMatched(host) && 
	    (type==null || hk.getType().equals(type)))){
	  foo[j++]=hk;
	}
      }
      return foo;
    }
  }
  public void remove(String host, String type){
    remove(host, type, null);
  }
  public void remove(String host, String type, byte[] key){
    boolean sync=false;
    synchronized(pool){
    for(int i=0; i<pool.size(); i++){
      HostKey hk=(HostKey)(pool.elementAt(i));
      if(host==null ||
	 (hk.isMatched(host) && 
	  (type==null || (hk.getType().equals(type) &&
			  (key==null || Util.array_equals(key, hk.key)))))){
        String hosts=hk.getHost();
        if(hosts.equals(host) || 
           ((hk instanceof HashedHostKey) &&
            ((HashedHostKey)hk).isHashed())){
          pool.removeElement(hk);
        }
        else{
          hk.host=deleteSubString(hosts, host);
        }
	sync=true;
      }
    }
    }
    if(sync){
      try{sync();}catch(Exception e){};
    }
  }

  protected void sync() throws IOException { 
    if(known_hosts!=null)
      sync(known_hosts); 
  }
  protected synchronized void sync(String foo) throws IOException {
    if(foo==null) return;
    FileOutputStream fos=new FileOutputStream(foo);
    dump(fos);
    fos.close();
  }

  private static final byte[] space={(byte)0x20};
  private static final byte[] cr=Util.str2byte("\n");
  void dump(OutputStream out) throws IOException {
    try{
      HostKey hk;
      synchronized(pool){
      for(int i=0; i<pool.size(); i++){
        hk=(HostKey)(pool.elementAt(i));
        //hk.dump(out);
	String host=hk.getHost();
	String type=hk.getType();
	if(type.equals("UNKNOWN")){
	  out.write(Util.str2byte(host));
	  out.write(cr);
	  continue;
	}
	out.write(Util.str2byte(host));
	out.write(space);
	out.write(Util.str2byte(type));
	out.write(space);
	out.write(Util.str2byte(hk.getKey()));
	out.write(cr);
      }
      }
    }
    catch(Exception e){
      System.err.println(e);
    }
  }
  private int getType(byte[] key){
    if(key[8]=='d') return HostKey.SSHDSS;
    if(key[8]=='r') return HostKey.SSHRSA;
    return HostKey.UNKNOWN;
  }
  private String deleteSubString(String hosts, String host){
    int i=0;
    int hostlen=host.length();
    int hostslen=hosts.length();
    int j;
    while(i<hostslen){
      j=hosts.indexOf(',', i);
      if(j==-1) break;
      if(!host.equals(hosts.substring(i, j))){
        i=j+1;	  
        continue;
      }
      return hosts.substring(0, i)+hosts.substring(j+1);
    }
    if(hosts.endsWith(host) && hostslen-i==hostlen){
      return hosts.substring(0, (hostlen==hostslen) ? 0 :hostslen-hostlen-1);
    }
    return hosts;
  }

  private synchronized MAC getHMACSHA1(){
    if(hmacsha1==null){
      try{
        Class c=Class.forName(jsch.getConfig("hmac-sha1"));
        hmacsha1=(MAC)(c.newInstance());
      }
      catch(Exception e){ 
        System.err.println("hmacsha1: "+e); 
      }
    }
    return hmacsha1;
  }

  HostKey createHashedHostKey(String host, byte[]key) throws JSchException {
    HashedHostKey hhk=new HashedHostKey(host, key);
    hhk.hash();
    return hhk;
  } 
  class HashedHostKey extends HostKey{
    private static final String HASH_MAGIC="|1|";
    private static final String HASH_DELIM="|";

    private boolean hashed=false;
    byte[] salt=null;
    byte[] hash=null;


    HashedHostKey(String host, byte[] key) throws JSchException {
      this(host, GUESS, key);
    }
    HashedHostKey(String host, int type, byte[] key) throws JSchException {
      super(host, type, key);
      if(this.host.startsWith(HASH_MAGIC) &&
         this.host.substring(HASH_MAGIC.length()).indexOf(HASH_DELIM)>0){
        String data=this.host.substring(HASH_MAGIC.length());
        String _salt=data.substring(0, data.indexOf(HASH_DELIM));
        String _hash=data.substring(data.indexOf(HASH_DELIM)+1);
        salt=Util.fromBase64(Util.str2byte(_salt), 0, _salt.length());
        hash=Util.fromBase64(Util.str2byte(_hash), 0, _hash.length());
        if(salt.length!=20 ||  // block size of hmac-sha1
           hash.length!=20){
          salt=null;
          hash=null;
          return;
        }
        hashed=true;
      }
    }

    boolean isMatched(String _host){
      if(!hashed){
        return super.isMatched(_host);
      }
      MAC macsha1=getHMACSHA1();
      try{
        synchronized(macsha1){
          macsha1.init(salt);
          byte[] foo=Util.str2byte(_host);
          macsha1.update(foo, 0, foo.length);
          byte[] bar=new byte[macsha1.getBlockSize()];
          macsha1.doFinal(bar, 0);
          return Util.array_equals(hash, bar);
        }
      }
      catch(Exception e){
        System.out.println(e);
      }
      return false;
    }

    boolean isHashed(){
      return hashed;
    }

    void hash(){
      if(hashed)
        return;
      MAC macsha1=getHMACSHA1();
      if(salt==null){
        Random random=Session.random;
        synchronized(random){
          salt=new byte[macsha1.getBlockSize()];
          random.fill(salt, 0, salt.length);
        }
      }
      try{
        synchronized(macsha1){
          macsha1.init(salt);
          byte[] foo=Util.str2byte(host);
          macsha1.update(foo, 0, foo.length);
          hash=new byte[macsha1.getBlockSize()];
          macsha1.doFinal(hash, 0);
        }
      }
      catch(Exception e){
      }
      host=HASH_MAGIC+Util.byte2str(Util.toBase64(salt, 0, salt.length))+
        HASH_DELIM+Util.byte2str(Util.toBase64(hash, 0, hash.length));
      hashed=true;
    }
  }
}
