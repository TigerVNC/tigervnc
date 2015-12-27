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

package com.jcraft.jsch.jce;

import java.math.BigInteger;
import java.security.*;
import java.security.spec.*;
import com.jcraft.jsch.Buffer;

public class SignatureECDSA implements com.jcraft.jsch.SignatureECDSA {

  Signature signature;
  KeyFactory keyFactory;

  public void init() throws Exception{
    signature=java.security.Signature.getInstance("SHA256withECDSA");
    keyFactory=KeyFactory.getInstance("EC");
  }     

  public void setPubKey(byte[] r, byte[] s) throws Exception{

    // r and s must be unsigned values.
    r=insert0(r);
    s=insert0(s);

    String name="secp256r1";
    if(r.length>=64) name="secp521r1";
    else if(r.length>=48) name="secp384r1";

    AlgorithmParameters param = AlgorithmParameters.getInstance("EC");
    param.init(new ECGenParameterSpec(name));
    ECParameterSpec ecparam =
      (ECParameterSpec)param.getParameterSpec(ECParameterSpec.class);
    ECPoint w = new ECPoint(new BigInteger(1, r), new BigInteger(1, s));
    PublicKey pubKey = 
      keyFactory.generatePublic(new ECPublicKeySpec(w, ecparam));
    signature.initVerify(pubKey);
  }

  public void setPrvKey(byte[] d) throws Exception{

    // d must be unsigned value.
    d=insert0(d);

    String name="secp256r1";
    if(d.length>=64) name="secp521r1";
    else if(d.length>=48) name="secp384r1";

    AlgorithmParameters param = AlgorithmParameters.getInstance("EC");
    param.init(new ECGenParameterSpec(name));
    ECParameterSpec ecparam =
      (ECParameterSpec)param.getParameterSpec(ECParameterSpec.class);
    BigInteger _d = new BigInteger(1, d);
    PrivateKey prvKey = 
      keyFactory.generatePrivate(new ECPrivateKeySpec(_d, ecparam));
    signature.initSign(prvKey);
  }
  public byte[] sign() throws Exception{
    byte[] sig=signature.sign();

    // It seems that the output from SunEC is in ASN.1,
    // so we have to convert it.
    if(sig[0]==0x30 &&                                      // in ASN.1
       ((sig[1]+2 == sig.length) ||
        ((sig[1]&0x80)!=0 && (sig[2]&0xff)+3==sig.length))){// 2bytes for len

      int index=3;
      if((sig[1]&0x80)!=0 && (sig[2]&0xff)+3==sig.length)
        index=4;

      byte[] r = new byte[sig[index]];
      byte[] s = new byte[sig[index+2+sig[index]]];
      System.arraycopy(sig, index+1, r, 0, r.length);
      System.arraycopy(sig, index+3+sig[index], s, 0, s.length);

      r = chop0(r);
      s = chop0(s);

      Buffer buf = new Buffer();
      buf.putMPInt(r);
      buf.putMPInt(s);

      sig=new byte[buf.getLength()];
      buf.setOffSet(0);
      buf.getByte(sig);
    }

    return sig;
  }
  public void update(byte[] foo) throws Exception{
   signature.update(foo);
  }
  public boolean verify(byte[] sig) throws Exception{

    // It seems that SunEC expects ASN.1 data,
    // so we have to convert it.
    if(!(sig[0]==0x30 &&                                    // not in ASN.1
         ((sig[1]+2 == sig.length) ||
          ((sig[1]&0x80)!=0 && (sig[2]&0xff)+3==sig.length)))) {
      Buffer b = new Buffer(sig);

      b.getString();  // ecdsa-sha2-nistp256
      b.getInt();

      byte[] r = b.getMPInt();
      byte[] s = b.getMPInt();

      r=insert0(r);
      s=insert0(s);

      byte[] asn1 = null;
      if(r.length<64){
        asn1 = new byte[6+r.length+s.length];
        asn1[0] = (byte)0x30;
        asn1[1] = (byte)(4+r.length+s.length);
        asn1[2] = (byte)0x02;
        asn1[3] = (byte)r.length;
        System.arraycopy(r, 0, asn1, 4, r.length);
        asn1[r.length+4] = (byte)0x02;
        asn1[r.length+5] = (byte)s.length;
        System.arraycopy(s, 0, asn1, (6+r.length), s.length);
      }
      else {
        asn1 = new byte[6+r.length+s.length+1];
        asn1[0] = (byte)0x30;
        asn1[1] = (byte)0x81;
        asn1[2] = (byte)(4+r.length+s.length);
        asn1[3] = (byte)0x02;
        asn1[4] = (byte)r.length;
        System.arraycopy(r, 0, asn1, 5, r.length);
        asn1[r.length+5] = (byte)0x02;
        asn1[r.length+6] = (byte)s.length;
        System.arraycopy(s, 0, asn1, (7+r.length), s.length);
      }
      sig=asn1;
    }

    return signature.verify(sig); 
  }

  private byte[] insert0(byte[] buf){
    if ((buf[0] & 0x80) == 0) return buf;
    byte[] tmp = new byte[buf.length+1];
    System.arraycopy(buf, 0, tmp, 1, buf.length);
    bzero(buf);
    return tmp;
  }
  private byte[] chop0(byte[] buf){
    if(buf[0]!=0) return buf;
    byte[] tmp = new byte[buf.length-1];
    System.arraycopy(buf, 1, tmp, 0, tmp.length);
    bzero(buf);
    return tmp;
  }

  private void bzero(byte[] buf){
    for(int i = 0; i<buf.length; i++) buf[i]=0;
  }
}
