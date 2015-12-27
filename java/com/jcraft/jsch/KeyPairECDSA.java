/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2015 ymnk, JCraft,Inc. All rights reserved.

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

public class KeyPairECDSA extends KeyPair{

  private static byte[][] oids = {
    {(byte)0x06, (byte)0x08, (byte)0x2a, (byte)0x86, (byte)0x48, // 256
     (byte)0xce, (byte)0x3d, (byte)0x03, (byte)0x01, (byte)0x07},
    {(byte)0x06, (byte)0x05, (byte)0x2b, (byte)0x81, (byte)0x04, // 384
     (byte)0x00, (byte)0x22},
    {(byte)0x06, (byte)0x05, (byte)0x2b, (byte)0x81, (byte)0x04, //521
     (byte)0x00, (byte)0x23},
  };

  private static String[] names = {
    "nistp256", "nistp384", "nistp521"
  };

  private byte[] name=Util.str2byte(names[0]);
  private byte[] r_array;
  private byte[] s_array;
  private byte[] prv_array;

  private int key_size=256;

  public KeyPairECDSA(JSch jsch){
    this(jsch, null, null, null, null);
  }

  public KeyPairECDSA(JSch jsch,
                      byte[] name,
                      byte[] r_array,
                      byte[] s_array,
                      byte[] prv_array){
    super(jsch);
    if(name!=null)
      this.name = name;
    this.r_array = r_array;
    this.s_array = s_array;
    this.prv_array = prv_array;
    if(prv_array!=null)
      key_size = prv_array.length>=64 ? 521 : 
                  (prv_array.length>=48 ? 384 : 256);
  }

  void generate(int key_size) throws JSchException{
    this.key_size=key_size;
    try{
      Class c=Class.forName(jsch.getConfig("keypairgen.ecdsa"));
      KeyPairGenECDSA keypairgen=(KeyPairGenECDSA)(c.newInstance());
      keypairgen.init(key_size);
      prv_array=keypairgen.getD();
      r_array=keypairgen.getR();
      s_array=keypairgen.getS();
      name=Util.str2byte(names[prv_array.length>=64 ? 2 :
                               (prv_array.length>=48 ? 1 : 0)]);
      keypairgen=null;
    }
    catch(Exception e){
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }
  }

  private static final byte[] begin = 
    Util.str2byte("-----BEGIN EC PRIVATE KEY-----");
  private static final byte[] end =
    Util.str2byte("-----END EC PRIVATE KEY-----");

  byte[] getBegin(){ return begin; }
  byte[] getEnd(){ return end; }

  byte[] getPrivateKey(){

    byte[] tmp = new byte[1]; tmp[0]=1;

    byte[] oid = oids[
                      (r_array.length>=64) ? 2 :
                       ((r_array.length>=48) ? 1 : 0)
                     ];

    byte[] point = toPoint(r_array, s_array);

    int bar = ((point.length+1)&0x80)==0 ? 3 : 4;
    byte[] foo = new byte[point.length+bar];
    System.arraycopy(point, 0, foo, bar, point.length);
    foo[0]=0x03;                     // BITSTRING 
    if(bar==3){
      foo[1]=(byte)(point.length+1);
    }
    else {
      foo[1]=(byte)0x81;
      foo[2]=(byte)(point.length+1);
    }
    point = foo;

    int content=
      1+countLength(tmp.length) + tmp.length +
      1+countLength(prv_array.length) + prv_array.length +
      1+countLength(oid.length) + oid.length +
      1+countLength(point.length) + point.length;

    int total=
      1+countLength(content)+content;   // SEQUENCE

    byte[] plain=new byte[total];
    int index=0;
    index=writeSEQUENCE(plain, index, content);
    index=writeINTEGER(plain, index, tmp);
    index=writeOCTETSTRING(plain, index, prv_array);
    index=writeDATA(plain, (byte)0xa0, index, oid);
    index=writeDATA(plain, (byte)0xa1, index, point);

    return plain;
  }

  boolean parse(byte[] plain){
    try{

      if(vendor==VENDOR_FSECURE){
        /*
	if(plain[0]!=0x30){              // FSecure
	  return true;
	}
	return false;
        */
	return false;
      }
      else if(vendor==VENDOR_PUTTY){
        /*
        Buffer buf=new Buffer(plain);
        buf.skip(plain.length);

        try {
          byte[][] tmp = buf.getBytes(1, "");
          prv_array = tmp[0];
        }
        catch(JSchException e){
          return false;
        }

        return true;
        */
	return false;
      }

      int index=0;
      int length=0;

      if(plain[index]!=0x30)return false;
      index++; // SEQUENCE
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }

      if(plain[index]!=0x02)return false;
      index++; // INTEGER

      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }

      index+=length;
      index++;   // 0x04

      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }

      prv_array=new byte[length];
      System.arraycopy(plain, index, prv_array, 0, length);

      index+=length;

      index++;  // 0xa0

      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }

      byte[] oid_array=new byte[length];
      System.arraycopy(plain, index, oid_array, 0, length);
      index+=length;

      for(int i = 0; i<oids.length; i++){
        if(Util.array_equals(oids[i], oid_array)){
          name = Util.str2byte(names[i]);
          break;
        }
      }

      index++;  // 0xa1

      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }

      byte[] Q_array=new byte[length];
      System.arraycopy(plain, index, Q_array, 0, length);
      index+=length;

      byte[][] tmp = fromPoint(Q_array);
      r_array = tmp[0];
      s_array = tmp[1];

      if(prv_array!=null)
        key_size = prv_array.length>=64 ? 521 : 
                    (prv_array.length>=48 ? 384 : 256);
    }
    catch(Exception e){
      //System.err.println(e);
      //e.printStackTrace();
      return false;
    }
    return true;
  }

  public byte[] getPublicKeyBlob(){
    byte[] foo = super.getPublicKeyBlob();

    if(foo!=null) return foo;

    if(r_array==null) return null;

    byte[][] tmp = new byte[3][];
    tmp[0] = Util.str2byte("ecdsa-sha2-"+new String(name));
    tmp[1] = name;
    tmp[2] = new byte[1+r_array.length+s_array.length];
    tmp[2][0] = 4;   // POINT_CONVERSION_UNCOMPRESSED
    System.arraycopy(r_array, 0, tmp[2], 1, r_array.length);
    System.arraycopy(s_array, 0, tmp[2], 1+r_array.length, s_array.length);

    return Buffer.fromBytes(tmp).buffer;
  }

  byte[] getKeyTypeName(){
    return Util.str2byte("ecdsa-sha2-"+new String(name));
  }
  public int getKeyType(){
    return ECDSA;
  }
  public int getKeySize(){
    return key_size;
  }

  public byte[] getSignature(byte[] data){
    try{      
      Class c=Class.forName((String)jsch.getConfig("signature.ecdsa"));
      SignatureECDSA ecdsa=(SignatureECDSA)(c.newInstance());
      ecdsa.init();
      ecdsa.setPrvKey(prv_array);

      ecdsa.update(data);
      byte[] sig = ecdsa.sign();

      byte[][] tmp = new byte[2][];
      tmp[0] = Util.str2byte("ecdsa-sha2-"+new String(name));
      tmp[1] = sig;
      return Buffer.fromBytes(tmp).buffer;
    }
    catch(Exception e){
      //System.err.println("e "+e);
    }
    return null;
  }

  public Signature getVerifier(){
    try{      
      Class c=Class.forName((String)jsch.getConfig("signature.ecdsa"));
      final SignatureECDSA ecdsa=(SignatureECDSA)(c.newInstance());
      ecdsa.init();

      if(r_array == null && s_array == null && getPublicKeyBlob()!=null){
        Buffer buf = new Buffer(getPublicKeyBlob());
        buf.getString();    // ecdsa-sha2-nistp256
        buf.getString();    // nistp256
        byte[][] tmp = fromPoint(buf.getString());
        r_array = tmp[0];
        s_array = tmp[1];
      } 
      ecdsa.setPubKey(r_array, s_array);
      return ecdsa;
    }
    catch(Exception e){
      //System.err.println("e "+e);
    }
    return null;
  }

  static KeyPair fromSSHAgent(JSch jsch, Buffer buf) throws JSchException {

    byte[][] tmp = buf.getBytes(5, "invalid key format");

    byte[] name = tmp[1];       // nistp256
    byte[][] foo = fromPoint(tmp[2]);
    byte[] r_array = foo[0];
    byte[] s_array = foo[1];

    byte[] prv_array = tmp[3];
    KeyPairECDSA kpair = new KeyPairECDSA(jsch,
                                          name,
                                          r_array, s_array,
                                          prv_array);
    kpair.publicKeyComment = new String(tmp[4]);
    kpair.vendor=VENDOR_OPENSSH;
    return kpair;
  }

  public byte[] forSSHAgent() throws JSchException {
    if(isEncrypted()){
      throw new JSchException("key is encrypted.");
    }
    Buffer buf = new Buffer();
    buf.putString(Util.str2byte("ecdsa-sha2-"+new String(name)));
    buf.putString(name);
    buf.putString(toPoint(r_array, s_array));
    buf.putString(prv_array);
    buf.putString(Util.str2byte(publicKeyComment));
    byte[] result = new byte[buf.getLength()];
    buf.getByte(result, 0, result.length);
    return result;
  }

  static byte[] toPoint(byte[] r_array, byte[] s_array) {
    byte[] tmp = new byte[1+r_array.length+s_array.length];
    tmp[0]=0x04;
    System.arraycopy(r_array, 0, tmp, 1, r_array.length);
    System.arraycopy(s_array, 0, tmp, 1+r_array.length, s_array.length);
    return tmp;
  }

  static byte[][] fromPoint(byte[] point) {
    int i = 0;
    while(point[i]!=4) i++;
    i++;
    byte[][] tmp = new byte[2][];
    byte[] r_array = new byte[(point.length-i)/2];
    byte[] s_array = new byte[(point.length-i)/2];
    // point[0] == 0x04 == POINT_CONVERSION_UNCOMPRESSED
    System.arraycopy(point, i, r_array, 0, r_array.length);
    System.arraycopy(point, i+r_array.length, s_array, 0, s_array.length);
    tmp[0] = r_array;
    tmp[1] = s_array;

    return tmp;
  }

  public void dispose(){
    super.dispose();
    Util.bzero(prv_array);
  }
}
