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

import java.security.*;
import java.security.interfaces.*;
import java.security.spec.*;
import com.jcraft.jsch.JSchException;

public class KeyPairGenECDSA implements com.jcraft.jsch.KeyPairGenECDSA {
  byte[] d;
  byte[] r;
  byte[] s;
  ECPublicKey pubKey;
  ECPrivateKey prvKey;
  ECParameterSpec params;
  public void init(int key_size) throws Exception {
    String name=null;
    if(key_size==256) name="secp256r1";
    else if(key_size==384) name="secp384r1";
    else if(key_size==521) name="secp521r1";
    else throw new JSchException("unsupported key size: "+key_size);

    for(int i = 0; i<1000; i++) {
      KeyPairGenerator kpg = KeyPairGenerator.getInstance("EC");
      ECGenParameterSpec ecsp = new ECGenParameterSpec(name);
      kpg.initialize(ecsp);
      KeyPair kp = kpg.genKeyPair();
      prvKey = (ECPrivateKey)kp.getPrivate();
      pubKey = (ECPublicKey)kp.getPublic();
      params=pubKey.getParams();
      d=((ECPrivateKey)prvKey).getS().toByteArray();
      ECPoint w = pubKey.getW();
      r = w.getAffineX().toByteArray();
      s = w.getAffineY().toByteArray();

      if(r.length!=s.length) continue;
      if(key_size==256 && r.length==32) break;
      if(key_size==384 && r.length==48) break;
      if(key_size==521 && r.length==66) break;
    }
    if(d.length<r.length){
      d=insert0(d);
    }
  }
  public byte[] getD(){return d;}
  public byte[] getR(){return r;}
  public byte[] getS(){return s;}
  ECPublicKey getPublicKey(){ return pubKey; }
  ECPrivateKey getPrivateKey(){ return prvKey; }

  private byte[] insert0(byte[] buf){
//    if ((buf[0] & 0x80) == 0) return buf;
    byte[] tmp = new byte[buf.length+1];
    System.arraycopy(buf, 0, tmp, 1, buf.length);
    bzero(buf);
    return tmp;
  }
  private byte[] chop0(byte[] buf){
    if(buf[0]!=0 || (buf[1]&0x80)==0) return buf;
    byte[] tmp = new byte[buf.length-1];
    System.arraycopy(buf, 1, tmp, 0, tmp.length);
    bzero(buf);
    return tmp;
  }
  private void bzero(byte[] buf){
    for(int i = 0; i<buf.length; i++) buf[i]=0;
  }
}
