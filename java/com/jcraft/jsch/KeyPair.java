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

import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.File;
import java.io.IOException;

public abstract class KeyPair{
  public static final int ERROR=0;
  public static final int DSA=1;
  public static final int RSA=2;
  public static final int ECDSA=3;
  public static final int UNKNOWN=4;

  static final int VENDOR_OPENSSH=0;
  static final int VENDOR_FSECURE=1;
  static final int VENDOR_PUTTY=2;
  static final int VENDOR_PKCS8=3;

  int vendor=VENDOR_OPENSSH;

  private static final byte[] cr=Util.str2byte("\n");

  public static KeyPair genKeyPair(JSch jsch, int type) throws JSchException{
    return genKeyPair(jsch, type, 1024);
  }
  public static KeyPair genKeyPair(JSch jsch, int type, int key_size) throws JSchException{
    KeyPair kpair=null;
    if(type==DSA){ kpair=new KeyPairDSA(jsch); }
    else if(type==RSA){ kpair=new KeyPairRSA(jsch); }
    else if(type==ECDSA){ kpair=new KeyPairECDSA(jsch); }
    if(kpair!=null){
      kpair.generate(key_size);
    }
    return kpair;
  }

  abstract void generate(int key_size) throws JSchException;

  abstract byte[] getBegin();
  abstract byte[] getEnd();
  abstract int getKeySize();

  public abstract byte[] getSignature(byte[] data);
  public abstract Signature getVerifier();

  public abstract byte[] forSSHAgent() throws JSchException;

  public String getPublicKeyComment(){
    return publicKeyComment;
  }

  public void setPublicKeyComment(String publicKeyComment){
    this.publicKeyComment = publicKeyComment;
  }

  protected String publicKeyComment = "no comment";

  JSch jsch=null;
  private Cipher cipher;
  private HASH hash;
  private Random random;

  private byte[] passphrase;

  public KeyPair(JSch jsch){
    this.jsch=jsch;
  }

  static byte[][] header={Util.str2byte("Proc-Type: 4,ENCRYPTED"),
                          Util.str2byte("DEK-Info: DES-EDE3-CBC,")};

  abstract byte[] getPrivateKey();

  /**
   * Writes the plain private key to the given output stream.
   * @param out output stream 
   * @see #writePrivateKey(java.io.OutputStream out, byte[] passphrase)
   */
  public void writePrivateKey(java.io.OutputStream out){
    this.writePrivateKey(out, null);
  }

  /**
   * Writes the cyphered private key to the given output stream.
   * @param out output stream 
   * @param passphrase a passphrase to encrypt the private key
   */
  public void writePrivateKey(java.io.OutputStream out, byte[] passphrase){
    if(passphrase == null)
      passphrase = this.passphrase;

    byte[] plain=getPrivateKey();
    byte[][] _iv=new byte[1][];
    byte[] encoded=encrypt(plain, _iv, passphrase);
    if(encoded!=plain)
      Util.bzero(plain);
    byte[] iv=_iv[0];
    byte[] prv=Util.toBase64(encoded, 0, encoded.length);

    try{
      out.write(getBegin()); out.write(cr);
      if(passphrase!=null){
	out.write(header[0]); out.write(cr);
	out.write(header[1]); 
	for(int i=0; i<iv.length; i++){
	  out.write(b2a((byte)((iv[i]>>>4)&0x0f)));
	  out.write(b2a((byte)(iv[i]&0x0f)));
	}
        out.write(cr);
	out.write(cr);
      }
      int i=0;
      while(i<prv.length){
	if(i+64<prv.length){
	  out.write(prv, i, 64);
	  out.write(cr);
	  i+=64;
	  continue;
	}
	out.write(prv, i, prv.length-i);
	out.write(cr);
	break;
      }
      out.write(getEnd()); out.write(cr);
      //out.close();
    }
    catch(Exception e){
    }
  }

  private static byte[] space=Util.str2byte(" ");

  abstract byte[] getKeyTypeName();
  public abstract int getKeyType();

  /**
   * Returns the blob of the public key.
   * @return blob of the public key
   */
  public byte[] getPublicKeyBlob() {
    // TODO JSchException should be thrown
    //if(publickeyblob == null)
    //  throw new JSchException("public-key blob is not available");
    return publickeyblob;
  }

  /**
   * Writes the public key with the specified comment to the output stream.
   * @param out output stream 
   * @param comment comment
   */
  public void writePublicKey(java.io.OutputStream out, String comment){
    byte[] pubblob=getPublicKeyBlob();
    byte[] pub=Util.toBase64(pubblob, 0, pubblob.length);
    try{
      out.write(getKeyTypeName()); out.write(space);
      out.write(pub, 0, pub.length); out.write(space);
      out.write(Util.str2byte(comment));
      out.write(cr);
    }
    catch(Exception e){
    }
  }

  /**
   * Writes the public key with the specified comment to the file.
   * @param name file name
   * @param comment comment
   * @see #writePublicKey(java.io.OutputStream out, String comment)
   */
  public void writePublicKey(String name, String comment) throws java.io.FileNotFoundException, java.io.IOException{
    FileOutputStream fos=new FileOutputStream(name);
    writePublicKey(fos, comment);
    fos.close();
  }

  /**
   * Writes the public key with the specified comment to the output stream in
   * the format defined in http://www.ietf.org/rfc/rfc4716.txt
   * @param out output stream 
   * @param comment comment
   */
  public void writeSECSHPublicKey(java.io.OutputStream out, String comment){
    byte[] pubblob=getPublicKeyBlob();
    byte[] pub=Util.toBase64(pubblob, 0, pubblob.length);
    try{
      out.write(Util.str2byte("---- BEGIN SSH2 PUBLIC KEY ----")); out.write(cr);
      out.write(Util.str2byte("Comment: \""+comment+"\"")); out.write(cr);
      int index=0;
      while(index<pub.length){
	int len=70;
	if((pub.length-index)<len)len=pub.length-index;
	out.write(pub, index, len); out.write(cr);
	index+=len;
      }
      out.write(Util.str2byte("---- END SSH2 PUBLIC KEY ----")); out.write(cr);
    }
    catch(Exception e){
    }
  }

  /**
   * Writes the public key with the specified comment to the output stream in
   * the format defined in http://www.ietf.org/rfc/rfc4716.txt
   * @param name file name
   * @param comment comment
   * @see #writeSECSHPublicKey(java.io.OutputStream out, String comment)
   */
  public void writeSECSHPublicKey(String name, String comment) throws java.io.FileNotFoundException, java.io.IOException{
    FileOutputStream fos=new FileOutputStream(name);
    writeSECSHPublicKey(fos, comment);
    fos.close();
  }

  /**
   * Writes the plain private key to the file.
   * @param name file name
   * @see #writePrivateKey(String name,  byte[] passphrase)
   */
  public void writePrivateKey(String name) throws java.io.FileNotFoundException, java.io.IOException{
    this.writePrivateKey(name, null);
  }

  /**
   * Writes the cyphered private key to the file.
   * @param name file name
   * @param passphrase a passphrase to encrypt the private key
   * @see #writePrivateKey(java.io.OutputStream out,  byte[] passphrase)
   */
  public void writePrivateKey(String name, byte[] passphrase) throws java.io.FileNotFoundException, java.io.IOException{
    FileOutputStream fos=new FileOutputStream(name);
    writePrivateKey(fos, passphrase);
    fos.close();
  }

  /**
   * Returns the finger-print of the public key.
   * @return finger print
   */
  public String getFingerPrint(){
    if(hash==null) hash=genHash();
    byte[] kblob=getPublicKeyBlob();
    if(kblob==null) return null;
    return Util.getFingerPrint(hash, kblob);
  }

  private byte[] encrypt(byte[] plain, byte[][] _iv, byte[] passphrase){
    if(passphrase==null) return plain;

    if(cipher==null) cipher=genCipher();
    byte[] iv=_iv[0]=new byte[cipher.getIVSize()];

    if(random==null) random=genRandom();
    random.fill(iv, 0, iv.length);

    byte[] key=genKey(passphrase, iv);
    byte[] encoded=plain;

    // PKCS#5Padding
    {
      //int bsize=cipher.getBlockSize();
      int bsize=cipher.getIVSize();
      byte[] foo=new byte[(encoded.length/bsize+1)*bsize];
      System.arraycopy(encoded, 0, foo, 0, encoded.length);
      int padding=bsize-encoded.length%bsize;
      for(int i=foo.length-1; (foo.length-padding)<=i; i--){
        foo[i]=(byte)padding;
      }
      encoded=foo;
    }

    try{
      cipher.init(Cipher.ENCRYPT_MODE, key, iv);
      cipher.update(encoded, 0, encoded.length, encoded, 0);
    }
    catch(Exception e){
      //System.err.println(e);
    }
    Util.bzero(key);
    return encoded;
  }

  abstract boolean parse(byte[] data);

  private byte[] decrypt(byte[] data, byte[] passphrase, byte[] iv){

    try{
      byte[] key=genKey(passphrase, iv);
      cipher.init(Cipher.DECRYPT_MODE, key, iv);
      Util.bzero(key);
      byte[] plain=new byte[data.length];
      cipher.update(data, 0, data.length, plain, 0);
      return plain;
    }
    catch(Exception e){
      //System.err.println(e);
    }
    return null;
  }

  int writeSEQUENCE(byte[] buf, int index, int len){
    buf[index++]=0x30;
    index=writeLength(buf, index, len);
    return index;
  }
  int writeINTEGER(byte[] buf, int index, byte[] data){
    buf[index++]=0x02;
    index=writeLength(buf, index, data.length);
    System.arraycopy(data, 0, buf, index, data.length);
    index+=data.length;
    return index;
  }

  int writeOCTETSTRING(byte[] buf, int index, byte[] data){
    buf[index++]=0x04;
    index=writeLength(buf, index, data.length);
    System.arraycopy(data, 0, buf, index, data.length);
    index+=data.length;
    return index;
  }

 int writeDATA(byte[] buf, byte n, int index, byte[] data){
    buf[index++]=n;
    index=writeLength(buf, index, data.length);
    System.arraycopy(data, 0, buf, index, data.length);
    index+=data.length;
    return index;
  }

  int countLength(int len){
    int i=1;
    if(len<=0x7f) return i;
    while(len>0){
      len>>>=8;
      i++;
    }
    return i;
  }

  int writeLength(byte[] data, int index, int len){
    int i=countLength(len)-1;
    if(i==0){
      data[index++]=(byte)len;
      return index;
    }
    data[index++]=(byte)(0x80|i);
    int j=index+i;
    while(i>0){
      data[index+i-1]=(byte)(len&0xff);
      len>>>=8;
      i--;
    }
    return j;
  }

  private Random genRandom(){
    if(random==null){
      try{
	Class c=Class.forName(jsch.getConfig("random"));
        random=(Random)(c.newInstance());
      }
      catch(Exception e){ System.err.println("connect: random "+e); }
    }
    return random;
  }

  private HASH genHash(){
    try{
      Class c=Class.forName(jsch.getConfig("md5"));
      hash=(HASH)(c.newInstance());
      hash.init();
    }
    catch(Exception e){
    }
    return hash;
  }
  private Cipher genCipher(){
    try{
      Class c;
      c=Class.forName(jsch.getConfig("3des-cbc"));
      cipher=(Cipher)(c.newInstance());
    }
    catch(Exception e){
    }
    return cipher;
  }

  /*
    hash is MD5
    h(0) <- hash(passphrase, iv);
    h(n) <- hash(h(n-1), passphrase, iv);
    key <- (h(0),...,h(n))[0,..,key.length];
  */
  synchronized byte[] genKey(byte[] passphrase, byte[] iv){
    if(cipher==null) cipher=genCipher();
    if(hash==null) hash=genHash();

    byte[] key=new byte[cipher.getBlockSize()];
    int hsize=hash.getBlockSize();
    byte[] hn=new byte[key.length/hsize*hsize+
		       (key.length%hsize==0?0:hsize)];
    try{
      byte[] tmp=null;
      if(vendor==VENDOR_OPENSSH){
	for(int index=0; index+hsize<=hn.length;){
	  if(tmp!=null){ hash.update(tmp, 0, tmp.length); }
	  hash.update(passphrase, 0, passphrase.length);
          hash.update(iv, 0, iv.length > 8 ? 8: iv.length);
	  tmp=hash.digest();
	  System.arraycopy(tmp, 0, hn, index, tmp.length);
	  index+=tmp.length;
	}
	System.arraycopy(hn, 0, key, 0, key.length); 
      }
      else if(vendor==VENDOR_FSECURE){
	for(int index=0; index+hsize<=hn.length;){
	  if(tmp!=null){ hash.update(tmp, 0, tmp.length); }
	  hash.update(passphrase, 0, passphrase.length);
	  tmp=hash.digest();
	  System.arraycopy(tmp, 0, hn, index, tmp.length);
	  index+=tmp.length;
	}
	System.arraycopy(hn, 0, key, 0, key.length); 
      }
      else if(vendor==VENDOR_PUTTY){
        Class c=Class.forName((String)jsch.getConfig("sha-1"));
        HASH sha1=(HASH)(c.newInstance());
        tmp = new byte[4];
        key = new byte[20*2];
        for(int i = 0; i < 2; i++){
          sha1.init();
          tmp[3]=(byte)i;
          sha1.update(tmp, 0, tmp.length);
          sha1.update(passphrase, 0, passphrase.length);
          System.arraycopy(sha1.digest(), 0, key, i*20, 20);
        }
      }
    }
    catch(Exception e){
      System.err.println(e);
    }
    return key;
  } 

  /**
   * @deprecated use #writePrivateKey(java.io.OutputStream out, byte[] passphrase)
   */
  public void setPassphrase(String passphrase){
    if(passphrase==null || passphrase.length()==0){
      setPassphrase((byte[])null);
    }
    else{
      setPassphrase(Util.str2byte(passphrase));
    }
  }

  /**
   * @deprecated use #writePrivateKey(String name, byte[] passphrase)
   */
  public void setPassphrase(byte[] passphrase){
    if(passphrase!=null && passphrase.length==0) 
      passphrase=null;
    this.passphrase=passphrase;
  }

  protected boolean encrypted=false;
  protected byte[] data=null;
  private byte[] iv=null;
  private byte[] publickeyblob=null;

  public boolean isEncrypted(){ return encrypted; }
  public boolean decrypt(String _passphrase){
    if(_passphrase==null || _passphrase.length()==0){
      return !encrypted;
    }
    return decrypt(Util.str2byte(_passphrase));
  }
  public boolean decrypt(byte[] _passphrase){

    if(!encrypted){
      return true;
    }
    if(_passphrase==null){
      return !encrypted;
    }
    byte[] bar=new byte[_passphrase.length];
    System.arraycopy(_passphrase, 0, bar, 0, bar.length);
    _passphrase=bar;
    byte[] foo=decrypt(data, _passphrase, iv);
    Util.bzero(_passphrase);
    if(parse(foo)){
      encrypted=false;
    }
    return !encrypted;
  }

  public static KeyPair load(JSch jsch, String prvkey) throws JSchException{
    String pubkey=prvkey+".pub";
    if(!new File(pubkey).exists()){
      pubkey=null;
    }
    return load(jsch, prvkey, pubkey);
  }
  public static KeyPair load(JSch jsch, String prvfile, String pubfile) throws JSchException{

    byte[] prvkey=null;
    byte[] pubkey=null;

    try{
      prvkey = Util.fromFile(prvfile);
    }
    catch(IOException e){
      throw new JSchException(e.toString(), (Throwable)e);
    }

    String _pubfile=pubfile;
    if(pubfile==null){
      _pubfile=prvfile+".pub";
    }

    try{
      pubkey = Util.fromFile(_pubfile);
    }
    catch(IOException e){
      if(pubfile!=null){  
        throw new JSchException(e.toString(), (Throwable)e);
      }
    }

    try {
      return load(jsch, prvkey, pubkey);
    }
    finally {
      Util.bzero(prvkey);
    }
  }

  public static KeyPair load(JSch jsch, byte[] prvkey, byte[] pubkey) throws JSchException{

    byte[] iv=new byte[8];       // 8
    boolean encrypted=true;
    byte[] data=null;

    byte[] publickeyblob=null;

    int type=ERROR;
    int vendor=VENDOR_OPENSSH;
    String publicKeyComment = "";
    Cipher cipher=null;

    // prvkey from "ssh-add" command on the remote.
    if(pubkey==null &&
       prvkey!=null && 
       (prvkey.length>11 &&
        prvkey[0]==0 && prvkey[1]==0 && prvkey[2]==0 &&
        (prvkey[3]==7 || prvkey[3]==19))){

      Buffer buf=new Buffer(prvkey);
      buf.skip(prvkey.length);  // for using Buffer#available()
      String _type = new String(buf.getString()); // ssh-rsa or ssh-dss
      buf.rewind();

      KeyPair kpair=null;
      if(_type.equals("ssh-rsa")){
        kpair=KeyPairRSA.fromSSHAgent(jsch, buf);
      }
      else if(_type.equals("ssh-dss")){
        kpair=KeyPairDSA.fromSSHAgent(jsch, buf);
      }
      else if(_type.equals("ecdsa-sha2-nistp256") ||
              _type.equals("ecdsa-sha2-nistp384") ||
              _type.equals("ecdsa-sha2-nistp512")){
        kpair=KeyPairECDSA.fromSSHAgent(jsch, buf);
      }
      else{
        throw new JSchException("privatekey: invalid key "+new String(prvkey, 4, 7));
      }
      return kpair;
    }

    try{
      byte[] buf=prvkey;

      if(buf!=null){
        KeyPair ppk = loadPPK(jsch, buf);
        if(ppk !=null)
          return ppk;
      }

      int len = (buf!=null ? buf.length : 0);
      int i=0;

      // skip garbage lines.
      while(i<len){
        if(buf[i] == '-' && i+4<len && 
           buf[i+1] == '-' && buf[i+2] == '-' && 
           buf[i+3] == '-' && buf[i+4] == '-'){
          break;
        }
        i++;
      }

      while(i<len){
        if(buf[i]=='B'&& i+3<len && buf[i+1]=='E'&& buf[i+2]=='G'&& buf[i+3]=='I'){
          i+=6;
          if(i+2 >= len)
	    throw new JSchException("invalid privatekey: "+prvkey);
          if(buf[i]=='D'&& buf[i+1]=='S'&& buf[i+2]=='A'){ type=DSA; }
	  else if(buf[i]=='R'&& buf[i+1]=='S'&& buf[i+2]=='A'){ type=RSA; }
	  else if(buf[i]=='E'&& buf[i+1]=='C'){ type=ECDSA; }
	  else if(buf[i]=='S'&& buf[i+1]=='S'&& buf[i+2]=='H'){ // FSecure
	    type=UNKNOWN;
	    vendor=VENDOR_FSECURE;
	  }
	  else if(i+6 < len &&
                  buf[i]=='P' && buf[i+1]=='R' &&
                  buf[i+2]=='I' && buf[i+3]=='V' &&
                  buf[i+4]=='A' && buf[i+5]=='T' && buf[i+6]=='E'){
	    type=UNKNOWN;
	    vendor=VENDOR_PKCS8;
            encrypted=false;
            i+=3;
	  }
	  else if(i+8 < len &&
                  buf[i]=='E' && buf[i+1]=='N' &&
                  buf[i+2]=='C' && buf[i+3]=='R' &&
                  buf[i+4]=='Y' && buf[i+5]=='P' && buf[i+6]=='T' &&
                  buf[i+7]=='E' && buf[i+8]=='D'){
	    type=UNKNOWN;
	    vendor=VENDOR_PKCS8;
            i+=5;
	  }
	  else{
	    throw new JSchException("invalid privatekey: "+prvkey);
	  }
          i+=3;
	  continue;
	}
        if(buf[i]=='A'&& i+7<len && buf[i+1]=='E'&& buf[i+2]=='S'&& buf[i+3]=='-' && 
           buf[i+4]=='2'&& buf[i+5]=='5'&& buf[i+6]=='6'&& buf[i+7]=='-'){
          i+=8;
          if(Session.checkCipher((String)jsch.getConfig("aes256-cbc"))){
            Class c=Class.forName((String)jsch.getConfig("aes256-cbc"));
            cipher=(Cipher)(c.newInstance());
            // key=new byte[cipher.getBlockSize()];
            iv=new byte[cipher.getIVSize()];
          }
          else{
            throw new JSchException("privatekey: aes256-cbc is not available "+prvkey);
          }
          continue;
        }
        if(buf[i]=='A'&& i+7<len && buf[i+1]=='E'&& buf[i+2]=='S'&& buf[i+3]=='-' && 
           buf[i+4]=='1'&& buf[i+5]=='9'&& buf[i+6]=='2'&& buf[i+7]=='-'){
          i+=8;
          if(Session.checkCipher((String)jsch.getConfig("aes192-cbc"))){
            Class c=Class.forName((String)jsch.getConfig("aes192-cbc"));
            cipher=(Cipher)(c.newInstance());
            // key=new byte[cipher.getBlockSize()];
            iv=new byte[cipher.getIVSize()];
          }
          else{
            throw new JSchException("privatekey: aes192-cbc is not available "+prvkey);
          }
          continue;
        }
        if(buf[i]=='A'&& i+7<len && buf[i+1]=='E'&& buf[i+2]=='S'&& buf[i+3]=='-' && 
           buf[i+4]=='1'&& buf[i+5]=='2'&& buf[i+6]=='8'&& buf[i+7]=='-'){
          i+=8;
          if(Session.checkCipher((String)jsch.getConfig("aes128-cbc"))){
            Class c=Class.forName((String)jsch.getConfig("aes128-cbc"));
            cipher=(Cipher)(c.newInstance());
            // key=new byte[cipher.getBlockSize()];
            iv=new byte[cipher.getIVSize()];
          }
          else{
            throw new JSchException("privatekey: aes128-cbc is not available "+prvkey);
          }
          continue;
        }
        if(buf[i]=='C'&& i+3<len && buf[i+1]=='B'&& buf[i+2]=='C'&& buf[i+3]==','){
          i+=4;
	  for(int ii=0; ii<iv.length; ii++){
            iv[ii]=(byte)(((a2b(buf[i++])<<4)&0xf0)+(a2b(buf[i++])&0xf));
  	  }
	  continue;
	}
	if(buf[i]==0x0d && i+1<buf.length && buf[i+1]==0x0a){
	  i++;
	  continue;
	}
	if(buf[i]==0x0a && i+1<buf.length){
	  if(buf[i+1]==0x0a){ i+=2; break; }
	  if(buf[i+1]==0x0d &&
	     i+2<buf.length && buf[i+2]==0x0a){
	     i+=3; break;
	  }
	  boolean inheader=false;
	  for(int j=i+1; j<buf.length; j++){
	    if(buf[j]==0x0a) break;
	    //if(buf[j]==0x0d) break;
	    if(buf[j]==':'){inheader=true; break;}
	  }
	  if(!inheader){
	    i++; 
	    if(vendor!=VENDOR_PKCS8)
              encrypted=false;    // no passphrase
	    break;
	  }
	}
	i++;
      }

      if(buf!=null){

        if(type==ERROR){
          throw new JSchException("invalid privatekey: "+prvkey);
        }

        int start = i;
        while(i < len){
          if(buf[i] == '-'){  break; }
          i++;
        }

        if((len-i) == 0 || (i-start) == 0){
          throw new JSchException("invalid privatekey: "+prvkey);
        }

        // The content of 'buf' will be changed, so it should be copied.
        byte[] tmp = new byte[i-start];
        System.arraycopy(buf, start, tmp, 0, tmp.length);
        byte[] _buf=tmp;

        start = 0;
        i = 0;

        int _len = _buf.length;
        while(i<_len){
          if(_buf[i]==0x0a){
            boolean xd=(_buf[i-1]==0x0d);
            // ignore 0x0a (or 0x0d0x0a)
            System.arraycopy(_buf, i+1, _buf, i-(xd ? 1 : 0), _len-(i+1));
            if(xd)_len--;
            _len--;
            continue;
          }
          if(_buf[i]=='-'){  break; }
          i++;
        }
        
        if(i-start > 0)
          data=Util.fromBase64(_buf, start, i-start);

        Util.bzero(_buf);
      }

      if(data!=null &&
         data.length>4 &&            // FSecure
	 data[0]==(byte)0x3f &&
	 data[1]==(byte)0x6f &&
	 data[2]==(byte)0xf9 &&
	 data[3]==(byte)0xeb){

	Buffer _buf=new Buffer(data);
	_buf.getInt();  // 0x3f6ff9be
	_buf.getInt();
	byte[]_type=_buf.getString();
	//System.err.println("type: "+new String(_type)); 
	String _cipher=Util.byte2str(_buf.getString());
	//System.err.println("cipher: "+_cipher); 
	if(_cipher.equals("3des-cbc")){
  	   _buf.getInt();
	   byte[] foo=new byte[data.length-_buf.getOffSet()];
	   _buf.getByte(foo);
	   data=foo;
	   encrypted=true;
	   throw new JSchException("unknown privatekey format: "+prvkey);
	}
	else if(_cipher.equals("none")){
  	   _buf.getInt();
  	   _buf.getInt();

           encrypted=false;

	   byte[] foo=new byte[data.length-_buf.getOffSet()];
	   _buf.getByte(foo);
	   data=foo;
	}
      }

      if(pubkey!=null){
	try{
	  buf=pubkey;
          len=buf.length;
	  if(buf.length>4 &&             // FSecure's public key
	     buf[0]=='-' && buf[1]=='-' && buf[2]=='-' && buf[3]=='-'){

	    boolean valid=true;
	    i=0;
	    do{i++;}while(buf.length>i && buf[i]!=0x0a);
	    if(buf.length<=i) {valid=false;}

	    while(valid){
	      if(buf[i]==0x0a){
		boolean inheader=false;
		for(int j=i+1; j<buf.length; j++){
		  if(buf[j]==0x0a) break;
		  if(buf[j]==':'){inheader=true; break;}
		}
		if(!inheader){
		  i++; 
		  break;
		}
	      }
	      i++;
	    }
	    if(buf.length<=i){valid=false;}

	    int start=i;
	    while(valid && i<len){
	      if(buf[i]==0x0a){
		System.arraycopy(buf, i+1, buf, i, len-i-1);
		len--;
		continue;
	      }
	      if(buf[i]=='-'){  break; }
	      i++;
	    }
	    if(valid){
	      publickeyblob=Util.fromBase64(buf, start, i-start);
	      if(prvkey==null || type==UNKNOWN){
		if(publickeyblob[8]=='d'){ type=DSA; }
		else if(publickeyblob[8]=='r'){ type=RSA; }
	      }
	    }
	  }
	  else{
	    if(buf[0]=='s'&& buf[1]=='s'&& buf[2]=='h' && buf[3]=='-'){
              if(prvkey==null &&
                 buf.length>7){
		if(buf[4]=='d'){ type=DSA; }
		else if(buf[4]=='r'){ type=RSA; }
              }
	      i=0;
	      while(i<len){ if(buf[i]==' ')break; i++;} i++;
	      if(i<len){
		int start=i;
		while(i<len){ if(buf[i]==' ')break; i++;}
		publickeyblob=Util.fromBase64(buf, start, i-start);
	      }
              if(i++<len){
                int start=i;
                while(i<len){ if(buf[i]=='\n')break; i++;}
                if(i>0 && buf[i-1]==0x0d) i--;
                if(start<i){
                  publicKeyComment = new String(buf, start, i-start);
                }
              } 
	    }
            else if(buf[0]=='e'&& buf[1]=='c'&& buf[2]=='d' && buf[3]=='s'){
              if(prvkey==null && buf.length>7){
               type=ECDSA;
              }
              i=0;
              while(i<len){ if(buf[i]==' ')break; i++;} i++;
              if(i<len){
                int start=i;
                while(i<len){ if(buf[i]==' ')break; i++;}
                publickeyblob=Util.fromBase64(buf, start, i-start);
              }
              if(i++<len){
                int start=i;
                while(i<len){ if(buf[i]=='\n')break; i++;}
                if(i>0 && buf[i-1]==0x0d) i--;
                if(start<i){
                  publicKeyComment = new String(buf, start, i-start);
                }
              } 
            }
	  }
	}
	catch(Exception ee){
	}
      }
    }
    catch(Exception e){
      if(e instanceof JSchException) throw (JSchException)e;
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }

    KeyPair kpair=null;
    if(type==DSA){ kpair=new KeyPairDSA(jsch); }
    else if(type==RSA){ kpair=new KeyPairRSA(jsch); }
    else if(type==ECDSA){ kpair=new KeyPairECDSA(jsch); }
    else if(vendor==VENDOR_PKCS8){ kpair = new KeyPairPKCS8(jsch); }

    if(kpair!=null){
      kpair.encrypted=encrypted;
      kpair.publickeyblob=publickeyblob;
      kpair.vendor=vendor;
      kpair.publicKeyComment=publicKeyComment;
      kpair.cipher=cipher;

      if(encrypted){
        kpair.encrypted=true;
	kpair.iv=iv;
	kpair.data=data;
      }
      else{
	if(kpair.parse(data)){
          kpair.encrypted=false;
	  return kpair;
	}
	else{
	  throw new JSchException("invalid privatekey: "+prvkey);
	}
      }
    }

    return kpair;
  }

  static private byte a2b(byte c){
    if('0'<=c&&c<='9') return (byte)(c-'0');
    return (byte)(c-'a'+10);
  }
  static private byte b2a(byte c){
    if(0<=c&&c<=9) return (byte)(c+'0');
    return (byte)(c-10+'A');
  }

  public void dispose(){
    Util.bzero(passphrase);
  }

  public void finalize (){
    dispose();
  }

  private static final String[] header1 = {
    "PuTTY-User-Key-File-2: ",
    "Encryption: ",
    "Comment: ",
    "Public-Lines: "
  };

  private static final String[] header2 = {
    "Private-Lines: "
  };

  private static final String[] header3 = {
    "Private-MAC: "
  };

  static KeyPair loadPPK(JSch jsch, byte[] buf) throws JSchException {
    byte[] pubkey = null;
    byte[] prvkey = null;
    int lines = 0;

    Buffer buffer = new Buffer(buf);
    java.util.Hashtable v = new java.util.Hashtable();

    while(true){
      if(!parseHeader(buffer, v))
        break;
    } 

    String typ = (String)v.get("PuTTY-User-Key-File-2");
    if(typ == null){
      return null;
    }

    lines = Integer.parseInt((String)v.get("Public-Lines"));
    pubkey = parseLines(buffer, lines); 

    while(true){
      if(!parseHeader(buffer, v))
        break;
    } 
    
    lines = Integer.parseInt((String)v.get("Private-Lines"));
    prvkey = parseLines(buffer, lines); 

    while(true){
      if(!parseHeader(buffer, v))
        break;
    } 

    prvkey = Util.fromBase64(prvkey, 0, prvkey.length);
    pubkey = Util.fromBase64(pubkey, 0, pubkey.length);

    KeyPair kpair = null;

    if(typ.equals("ssh-rsa")) {

      Buffer _buf = new Buffer(pubkey);
      _buf.skip(pubkey.length);

      int len = _buf.getInt();
      _buf.getByte(new byte[len]);             // ssh-rsa
      byte[] pub_array = new byte[_buf.getInt()];
      _buf.getByte(pub_array);
      byte[] n_array = new byte[_buf.getInt()];
      _buf.getByte(n_array);

      kpair = new KeyPairRSA(jsch, n_array, pub_array, null);
    }
    else if(typ.equals("ssh-dss")){
      Buffer _buf = new Buffer(pubkey);
      _buf.skip(pubkey.length);

      int len = _buf.getInt();
      _buf.getByte(new byte[len]);              // ssh-dss

      byte[] p_array = new byte[_buf.getInt()];
      _buf.getByte(p_array);
      byte[] q_array = new byte[_buf.getInt()];
      _buf.getByte(q_array);
      byte[] g_array = new byte[_buf.getInt()];
      _buf.getByte(g_array);
      byte[] y_array = new byte[_buf.getInt()];
      _buf.getByte(y_array);

      kpair = new KeyPairDSA(jsch, p_array, q_array, g_array, y_array, null);
    }
    else {
      return null;
    }

    if(kpair == null)
      return null;

    kpair.encrypted = !v.get("Encryption").equals("none");
    kpair.vendor = VENDOR_PUTTY;
    kpair.publicKeyComment = (String)v.get("Comment");
    if(kpair.encrypted){
      if(Session.checkCipher((String)jsch.getConfig("aes256-cbc"))){
        try {
          Class c=Class.forName((String)jsch.getConfig("aes256-cbc"));
          kpair.cipher=(Cipher)(c.newInstance());
          kpair.iv=new byte[kpair.cipher.getIVSize()];
        }
        catch(Exception e){
          throw new JSchException("The cipher 'aes256-cbc' is required, but it is not available.");
        }
      }
      else {
        throw new JSchException("The cipher 'aes256-cbc' is required, but it is not available.");
      }
      kpair.data = prvkey;
    }
    else {
      kpair.data = prvkey;
      kpair.parse(prvkey);
    }
    return kpair;
  }

  private static byte[] parseLines(Buffer buffer, int lines){
    byte[] buf = buffer.buffer;
    int index = buffer.index;
    byte[] data = null;

    int i = index;
    while(lines-->0){
      while(buf.length > i){
        if(buf[i++] == 0x0d){
          if(data == null){
            data = new byte[i - index - 1];
            System.arraycopy(buf, index, data, 0, i - index - 1);
          }
          else {
            byte[] tmp = new byte[data.length + i - index - 1];
            System.arraycopy(data, 0, tmp, 0, data.length);
            System.arraycopy(buf, index, tmp, data.length, i - index -1);
            for(int j = 0; j < data.length; j++) data[j] = 0; // clear
            data = tmp;
          } 
          break;
        }
      }
      if(buf[i]==0x0a)
        i++;
      index=i;
    }

    if(data != null)
      buffer.index = index;

    return data;
  }

  private static boolean parseHeader(Buffer buffer, java.util.Hashtable v){
    byte[] buf = buffer.buffer;
    int index = buffer.index;
    String key = null;
    String value = null;
    for(int i = index; i < buf.length; i++){
      if(buf[i] == 0x0d){
        break;
      }
      if(buf[i] == ':'){
        key = new String(buf, index, i - index);
        i++;
        if(i < buf.length && buf[i] == ' '){
          i++;
        }
        index = i;
        break;
      }
    }

    if(key == null)
      return false;

    for(int i = index; i < buf.length; i++){
      if(buf[i] == 0x0d){
        value = new String(buf, index, i - index);
        i++;
        if(i < buf.length && buf[i] == 0x0a){
          i++;
        }
        index = i;
        break;
      }
    }

    if(value != null){
      v.put(key, value);
      buffer.index = index;
    }

    return (key != null && value != null);
  }

  void copy(KeyPair kpair){
    this.publickeyblob=kpair.publickeyblob;
    this.vendor=kpair.vendor;
    this.publicKeyComment=kpair.publicKeyComment;
    this.cipher=kpair.cipher;
  }

  class ASN1Exception extends Exception {
  }

  class ASN1 {
    byte[] buf;
    int start;
    int length;
    ASN1(byte[] buf) throws ASN1Exception {
      this(buf, 0, buf.length);
    }
    ASN1(byte[] buf, int start, int length) throws ASN1Exception {
      this.buf = buf;
      this.start = start;
      this.length = length;
      if(start+length>buf.length)
        throw new ASN1Exception();
    }
    int getType() {
      return buf[start]&0xff;
    }
    boolean isSEQUENCE() {
      return getType()==(0x30&0xff);
    }
    boolean isINTEGER() {
      return getType()==(0x02&0xff);
    }
    boolean isOBJECT() {
      return getType()==(0x06&0xff);
    }
    boolean isOCTETSTRING() {
      return getType()==(0x04&0xff);
    }
    private int getLength(int[] indexp) {
      int index=indexp[0];
      int length=buf[index++]&0xff;
      if((length&0x80)!=0) {
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(buf[index++]&0xff); }
      }
      indexp[0]=index;
      return length;
    }
    byte[] getContent() {
      int[] indexp=new int[1];
      indexp[0]=start+1;
      int length = getLength(indexp);
      int index=indexp[0];
      byte[] tmp = new byte[length];
      System.arraycopy(buf, index, tmp, 0, tmp.length);
      return tmp;
    }
    ASN1[] getContents() throws ASN1Exception {
      int typ = buf[start];
      int[] indexp=new int[1];
      indexp[0]=start+1;
      int length = getLength(indexp);
      if(typ == 0x05){
        return new ASN1[0];
      }
      int index=indexp[0];
      java.util.Vector values = new java.util.Vector();
      while(length>0) {
        index++; length--;
        int tmp=index;
        indexp[0]=index;
        int l=getLength(indexp);
        index=indexp[0];
        length-=(index-tmp);
        values.addElement(new ASN1(buf, tmp-1, 1+(index-tmp)+l));
        index+=l;
        length-=l;
      }
      ASN1[] result = new ASN1[values.size()];
      for(int  i = 0; i <values.size(); i++) {
        result[i]=(ASN1)values.elementAt(i);
      }
      return result;
    }
  }
}
