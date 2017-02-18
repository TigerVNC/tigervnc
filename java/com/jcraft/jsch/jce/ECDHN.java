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
import javax.crypto.*;
import java.security.spec.*;
import java.security.interfaces.*;
 
public class ECDHN implements com.jcraft.jsch.ECDH {
  byte[] Q_array;
  ECPublicKey publicKey;

  private KeyAgreement myKeyAgree;
  public void init(int size) throws Exception{
    myKeyAgree = KeyAgreement.getInstance("ECDH");
    KeyPairGenECDSA kpair = new KeyPairGenECDSA();
    kpair.init(size);
    publicKey = kpair.getPublicKey();
    byte[] r = kpair.getR();
    byte[] s = kpair.getS();
    Q_array = toPoint(r, s);
    myKeyAgree.init(kpair.getPrivateKey());
  }

  public byte[] getQ() throws Exception{
    return Q_array;
  }

  public byte[] getSecret(byte[] r, byte[] s) throws Exception{

    KeyFactory kf = KeyFactory.getInstance("EC");
    ECPoint w = new ECPoint(new BigInteger(1, r), new BigInteger(1, s));
    ECPublicKeySpec spec = new ECPublicKeySpec(w, publicKey.getParams());
    PublicKey theirPublicKey = kf.generatePublic(spec);
    myKeyAgree.doPhase(theirPublicKey, true);
    return myKeyAgree.generateSecret();
  }

  private static BigInteger two = BigInteger.ONE.add(BigInteger.ONE);
  private static BigInteger three = two.add(BigInteger.ONE);

  // SEC 1: Elliptic Curve Cryptography, Version 2.0
  // http://www.secg.org/sec1-v2.pdf
  // 3.2.2.1 Elliptic Curve Public Key Validation Primitive
  public boolean validate(byte[] r, byte[] s) throws Exception{
    BigInteger x = new BigInteger(1, r);
    BigInteger y = new BigInteger(1, s);

    // Step.1
    //   Check that Q != O
    ECPoint w = new ECPoint(x, y);
    if(w.equals(ECPoint.POINT_INFINITY)){
      return false;
    }

    // Step.2
    // If T represents elliptic curve domain parameters over Fp,
    // check that xQ and yQ are integers in the interval [0, p-1],
    // and that:
    //   y^2 = x^3 + x*a + b (mod p)

    ECParameterSpec params = publicKey.getParams();
    EllipticCurve curve = params.getCurve();
    BigInteger p=((ECFieldFp)curve.getField()).getP(); //nistp should be Fp. 

    // xQ and yQ should be integers in the interval [0, p-1]
    BigInteger p_sub1=p.subtract(BigInteger.ONE);
    if(!(x.compareTo(p_sub1)<=0 && y.compareTo(p_sub1)<=0)){
      return false;
    }

    // y^2 = x^3 + x*a + b (mod p)
    BigInteger tmp=x.multiply(curve.getA()).
                     add(curve.getB()).
                     add(x.modPow(three, p)).
                     mod(p);
    BigInteger y_2=y.modPow(two, p);
    if(!(y_2.equals(tmp))){ 
      return false;
    }

    // Step.3
    //   Check that nQ = O.
    // Unfortunately, JCE does not provide the point multiplication method.
    /*
    if(!w.multiply(params.getOrder()).equals(ECPoint.POINT_INFINITY)){
      return false;
    }
    */
    return true;
  }

  private byte[] toPoint(byte[] r_array, byte[] s_array) {
    byte[] tmp = new byte[1+r_array.length+s_array.length];
    tmp[0]=0x04;
    System.arraycopy(r_array, 0, tmp, 1, r_array.length);
    System.arraycopy(s_array, 0, tmp, 1+r_array.length, s_array.length);
    return tmp;
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
