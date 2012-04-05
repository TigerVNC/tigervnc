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

package com.jcraft.jsch.jce;

import java.math.BigInteger;
import java.security.*;
import java.security.spec.*;

public class SignatureRSA implements com.jcraft.jsch.SignatureRSA{

  java.security.Signature signature;
  KeyFactory keyFactory;

  public void init() throws Exception{
    signature=java.security.Signature.getInstance("SHA1withRSA");
    keyFactory=KeyFactory.getInstance("RSA");
  }     
  public void setPubKey(byte[] e, byte[] n) throws Exception{
    RSAPublicKeySpec rsaPubKeySpec = 
	new RSAPublicKeySpec(new BigInteger(n),
			     new BigInteger(e));
    PublicKey pubKey=keyFactory.generatePublic(rsaPubKeySpec);
    signature.initVerify(pubKey);
  }
  public void setPrvKey(byte[] d, byte[] n) throws Exception{
    RSAPrivateKeySpec rsaPrivKeySpec = 
	new RSAPrivateKeySpec(new BigInteger(n),
			      new BigInteger(d));
    PrivateKey prvKey = keyFactory.generatePrivate(rsaPrivKeySpec);
    signature.initSign(prvKey);
  }
  public byte[] sign() throws Exception{
    byte[] sig=signature.sign();      
    return sig;
  }
  public void update(byte[] foo) throws Exception{
   signature.update(foo);
  }
  public boolean verify(byte[] sig) throws Exception{
    int i=0;
    int j=0;
    byte[] tmp;

    if(sig[0]==0 && sig[1]==0 && sig[2]==0){
    j=((sig[i++]<<24)&0xff000000)|((sig[i++]<<16)&0x00ff0000)|
	((sig[i++]<<8)&0x0000ff00)|((sig[i++])&0x000000ff);
    i+=j;
    j=((sig[i++]<<24)&0xff000000)|((sig[i++]<<16)&0x00ff0000)|
	((sig[i++]<<8)&0x0000ff00)|((sig[i++])&0x000000ff);
    tmp=new byte[j]; 
    System.arraycopy(sig, i, tmp, 0, j); sig=tmp;
    }
//System.err.println("j="+j+" "+Integer.toHexString(sig[0]&0xff));
    return signature.verify(sig);
  }
}
