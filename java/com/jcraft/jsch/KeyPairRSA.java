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

import java.math.BigInteger;

public class KeyPairRSA extends KeyPair{
  private byte[] n_array;   // modulus   p multiply q
  private byte[] pub_array; // e         
  private byte[] prv_array; // d         e^-1 mod (p-1)(q-1)

  private byte[] p_array;  // prime p
  private byte[] q_array;  // prime q
  private byte[] ep_array; // prime exponent p  dmp1 == prv mod (p-1)
  private byte[] eq_array; // prime exponent q  dmq1 == prv mod (q-1)
  private byte[] c_array;  // coefficient  iqmp == modinv(q, p) == q^-1 mod p

  private int key_size=1024;

  public KeyPairRSA(JSch jsch){
    this(jsch, null, null, null);
  }

  public KeyPairRSA(JSch jsch,
                    byte[] n_array,
                    byte[] pub_array,
                    byte[] prv_array){
    super(jsch);
    this.n_array = n_array;
    this.pub_array = pub_array;
    this.prv_array = prv_array;
    if(n_array!=null){
      key_size = (new java.math.BigInteger(n_array)).bitLength();
    }
  }

  @SuppressWarnings({"static"}) void generate(int key_size) throws JSchException{
    this.key_size=key_size;
    try{
      Class c=Class.forName(jsch.getConfig("keypairgen.rsa"));
      KeyPairGenRSA keypairgen=(KeyPairGenRSA)(c.newInstance());
      keypairgen.init(key_size);
      pub_array=keypairgen.getE();
      prv_array=keypairgen.getD();
      n_array=keypairgen.getN();

      p_array=keypairgen.getP();
      q_array=keypairgen.getQ();
      ep_array=keypairgen.getEP();
      eq_array=keypairgen.getEQ();
      c_array=keypairgen.getC();

      keypairgen=null;
    }
    catch(Exception e){
      //System.err.println("KeyPairRSA: "+e); 
      if(e instanceof Throwable)
        throw new JSchException(e.toString(), (Throwable)e);
      throw new JSchException(e.toString());
    }
  }

  private static final byte[] begin=Util.str2byte("-----BEGIN RSA PRIVATE KEY-----");
  private static final byte[] end=Util.str2byte("-----END RSA PRIVATE KEY-----");

  byte[] getBegin(){ return begin; }
  byte[] getEnd(){ return end; }

  byte[] getPrivateKey(){
    int content=
      1+countLength(1) + 1 +                           // INTEGER
      1+countLength(n_array.length) + n_array.length + // INTEGER  N
      1+countLength(pub_array.length) + pub_array.length + // INTEGER  pub
      1+countLength(prv_array.length) + prv_array.length+  // INTEGER  prv
      1+countLength(p_array.length) + p_array.length+      // INTEGER  p
      1+countLength(q_array.length) + q_array.length+      // INTEGER  q
      1+countLength(ep_array.length) + ep_array.length+    // INTEGER  ep
      1+countLength(eq_array.length) + eq_array.length+    // INTEGER  eq
      1+countLength(c_array.length) + c_array.length;      // INTEGER  c

    int total=
      1+countLength(content)+content;   // SEQUENCE

    byte[] plain=new byte[total];
    int index=0;
    index=writeSEQUENCE(plain, index, content);
    index=writeINTEGER(plain, index, new byte[1]);  // 0
    index=writeINTEGER(plain, index, n_array);
    index=writeINTEGER(plain, index, pub_array);
    index=writeINTEGER(plain, index, prv_array);
    index=writeINTEGER(plain, index, p_array);
    index=writeINTEGER(plain, index, q_array);
    index=writeINTEGER(plain, index, ep_array);
    index=writeINTEGER(plain, index, eq_array);
    index=writeINTEGER(plain, index, c_array);
    return plain;
  }

  boolean parse(byte [] plain){

    try{
      int index=0;
      int length=0;

      if(vendor==VENDOR_PUTTY){
        Buffer buf = new Buffer(plain);
        buf.skip(plain.length);

        try {
          byte[][] tmp = buf.getBytes(4, "");
          prv_array = tmp[0];
          p_array = tmp[1];
          q_array = tmp[2];
          c_array = tmp[3];
        }
        catch(JSchException e){
          return false;
        }

        getEPArray();
        getEQArray();

        return true;
      }

      if(vendor==VENDOR_FSECURE){
	if(plain[index]!=0x30){                  // FSecure
	  Buffer buf=new Buffer(plain);
	  pub_array=buf.getMPIntBits();
	  prv_array=buf.getMPIntBits();
	  n_array=buf.getMPIntBits();
	  byte[] u_array=buf.getMPIntBits();
	  p_array=buf.getMPIntBits();
	  q_array=buf.getMPIntBits();
          if(n_array!=null){
            key_size = (new java.math.BigInteger(n_array)).bitLength();
          }

          getEPArray();
          getEQArray();
          getCArray();

	  return true;
	}
	return false;
      }

      /*
        Key must be in the following ASN.1 DER encoding,
        RSAPrivateKey ::= SEQUENCE {
          version           Version,
          modulus           INTEGER,  -- n
          publicExponent    INTEGER,  -- e
          privateExponent   INTEGER,  -- d
          prime1            INTEGER,  -- p
          prime2            INTEGER,  -- q
          exponent1         INTEGER,  -- d mod (p-1)
          exponent2         INTEGER,  -- d mod (q-1)
          coefficient       INTEGER,  -- (inverse of q) mod p
          otherPrimeInfos   OtherPrimeInfos OPTIONAL
        }
      */

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

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      n_array=new byte[length];
      System.arraycopy(plain, index, n_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      pub_array=new byte[length];
      System.arraycopy(plain, index, pub_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      prv_array=new byte[length];
      System.arraycopy(plain, index, prv_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      p_array=new byte[length];
      System.arraycopy(plain, index, p_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      q_array=new byte[length];
      System.arraycopy(plain, index, q_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      ep_array=new byte[length];
      System.arraycopy(plain, index, ep_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      eq_array=new byte[length];
      System.arraycopy(plain, index, eq_array, 0, length);
      index+=length;

      index++;
      length=plain[index++]&0xff;
      if((length&0x80)!=0){
        int foo=length&0x7f; length=0;
        while(foo-->0){ length=(length<<8)+(plain[index++]&0xff); }
      }
      c_array=new byte[length];
      System.arraycopy(plain, index, c_array, 0, length);
      index+=length;

      if(n_array!=null){
        key_size = (new java.math.BigInteger(n_array)).bitLength();
      }

    }
    catch(Exception e){
      //System.err.println(e);
      return false;
    }
    return true;
  }

  public byte[] getPublicKeyBlob(){
    byte[] foo=super.getPublicKeyBlob();
    if(foo!=null) return foo;

    if(pub_array==null) return null;
    byte[][] tmp = new byte[3][];
    tmp[0] = sshrsa;
    tmp[1] = pub_array;
    tmp[2] = n_array;
    return Buffer.fromBytes(tmp).buffer;
  }

  private static final byte[] sshrsa=Util.str2byte("ssh-rsa");
  byte[] getKeyTypeName(){return sshrsa;}
  public int getKeyType(){return RSA;}

  public int getKeySize(){
    return key_size;
  }

  @SuppressWarnings({"static"}) public byte[] getSignature(byte[] data){
    try{      
      Class c=Class.forName((String)jsch.getConfig("signature.rsa"));
      SignatureRSA rsa=(SignatureRSA)(c.newInstance());
      rsa.init();
      rsa.setPrvKey(prv_array, n_array);

      rsa.update(data);
      byte[] sig = rsa.sign();
      byte[][] tmp = new byte[2][];
      tmp[0] = sshrsa;
      tmp[1] = sig;
      return Buffer.fromBytes(tmp).buffer;
    }
    catch(Exception e){
    }
    return null;
  }

  @SuppressWarnings({"static"}) public Signature getVerifier(){
    try{      
      Class c=Class.forName((String)jsch.getConfig("signature.rsa"));
      SignatureRSA rsa=(SignatureRSA)(c.newInstance());
      rsa.init();

      if(pub_array == null && n_array == null && getPublicKeyBlob()!=null){
        Buffer buf = new Buffer(getPublicKeyBlob());
        buf.getString();
        pub_array = buf.getString();
        n_array = buf.getString();
      } 

      rsa.setPubKey(pub_array, n_array);
      return rsa;
    }
    catch(Exception e){
    }
    return null;
  }

  static KeyPair fromSSHAgent(JSch jsch, Buffer buf) throws JSchException {

    byte[][] tmp = buf.getBytes(8, "invalid key format");

    byte[] n_array = tmp[1];
    byte[] pub_array = tmp[2];
    byte[] prv_array = tmp[3];
    KeyPairRSA kpair = new KeyPairRSA(jsch, n_array, pub_array, prv_array);
    kpair.c_array = tmp[4];     // iqmp
    kpair.p_array = tmp[5];
    kpair.q_array = tmp[6];
    kpair.publicKeyComment = new String(tmp[7]);
    kpair.vendor=VENDOR_OPENSSH;
    return kpair;
  }

  public byte[] forSSHAgent() throws JSchException {
    if(isEncrypted()){
      throw new JSchException("key is encrypted.");
    }
    Buffer buf = new Buffer();
    buf.putString(sshrsa);
    buf.putString(n_array);
    buf.putString(pub_array);
    buf.putString(prv_array);
    buf.putString(getCArray());
    buf.putString(p_array);
    buf.putString(q_array);
    buf.putString(Util.str2byte(publicKeyComment));
    byte[] result = new byte[buf.getLength()];
    buf.getByte(result, 0, result.length);
    return result;
  }

  private byte[] getEPArray(){
    if(ep_array==null){
      ep_array=(new BigInteger(prv_array)).mod(new BigInteger(p_array).subtract(BigInteger.ONE)).toByteArray();
    }
    return ep_array;
  } 

  private byte[] getEQArray(){
    if(eq_array==null){
      eq_array=(new BigInteger(prv_array)).mod(new BigInteger(q_array).subtract(BigInteger.ONE)).toByteArray();
    }
    return eq_array;
  } 

  private byte[] getCArray(){
    if(c_array==null){
      c_array=(new BigInteger(q_array)).modInverse(new BigInteger(p_array)).toByteArray();
    }
    return c_array;
  } 

  public void dispose(){
    super.dispose();
    Util.bzero(prv_array);
  }
}
