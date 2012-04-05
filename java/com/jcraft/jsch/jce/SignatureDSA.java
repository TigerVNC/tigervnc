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

public class SignatureDSA implements com.jcraft.jsch.SignatureDSA{

  java.security.Signature signature;
  KeyFactory keyFactory;

  public void init() throws Exception{
    signature=java.security.Signature.getInstance("SHA1withDSA");
    keyFactory=KeyFactory.getInstance("DSA");
  }     
  public void setPubKey(byte[] y, byte[] p, byte[] q, byte[] g) throws Exception{
    DSAPublicKeySpec dsaPubKeySpec = 
	new DSAPublicKeySpec(new BigInteger(y),
			     new BigInteger(p),
			     new BigInteger(q),
			     new BigInteger(g));
    PublicKey pubKey=keyFactory.generatePublic(dsaPubKeySpec);
    signature.initVerify(pubKey);
  }
  public void setPrvKey(byte[] x, byte[] p, byte[] q, byte[] g) throws Exception{
    DSAPrivateKeySpec dsaPrivKeySpec = 
	new DSAPrivateKeySpec(new BigInteger(x),
			      new BigInteger(p),
			      new BigInteger(q),
			      new BigInteger(g));
    PrivateKey prvKey = keyFactory.generatePrivate(dsaPrivKeySpec);
    signature.initSign(prvKey);
  }
  public byte[] sign() throws Exception{
    byte[] sig=signature.sign();      
/*
System.err.print("sign["+sig.length+"] ");
for(int i=0; i<sig.length;i++){
System.err.print(Integer.toHexString(sig[i]&0xff)+":");
}
System.err.println("");
*/
    // sig is in ASN.1
    // SEQUENCE::={ r INTEGER, s INTEGER }
    int len=0;	
    int index=3;
    len=sig[index++]&0xff;
//System.err.println("! len="+len);
    byte[] r=new byte[len];
    System.arraycopy(sig, index, r, 0, r.length);
    index=index+len+1;
    len=sig[index++]&0xff;
//System.err.println("!! len="+len);
    byte[] s=new byte[len];
    System.arraycopy(sig, index, s, 0, s.length);

    byte[] result=new byte[40];

    // result must be 40 bytes, but length of r and s may not be 20 bytes  

    System.arraycopy(r, (r.length>20)?1:0,
		     result, (r.length>20)?0:20-r.length,
		     (r.length>20)?20:r.length);
    System.arraycopy(s, (s.length>20)?1:0,
		     result, (s.length>20)?20:40-s.length,
		     (s.length>20)?20:s.length);
 
//  System.arraycopy(sig, (sig[3]==20?4:5), result, 0, 20);
//  System.arraycopy(sig, sig.length-20, result, 20, 20);

    return result;
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

    // ASN.1
    int frst=((sig[0]&0x80)!=0?1:0);
    int scnd=((sig[20]&0x80)!=0?1:0);
    //System.err.println("frst: "+frst+", scnd: "+scnd);

    int length=sig.length+6+frst+scnd;
    tmp=new byte[length];
    tmp[0]=(byte)0x30; tmp[1]=(byte)0x2c; 
    tmp[1]+=frst; tmp[1]+=scnd;
    tmp[2]=(byte)0x02; tmp[3]=(byte)0x14;
    tmp[3]+=frst;
    System.arraycopy(sig, 0, tmp, 4+frst, 20);
    tmp[4+tmp[3]]=(byte)0x02; tmp[5+tmp[3]]=(byte)0x14;
    tmp[5+tmp[3]]+=scnd;
    System.arraycopy(sig, 20, tmp, 6+tmp[3]+scnd, 20);
    sig=tmp;

/*
    tmp=new byte[sig.length+6];
    tmp[0]=(byte)0x30; tmp[1]=(byte)0x2c; 
    tmp[2]=(byte)0x02; tmp[3]=(byte)0x14;
    System.arraycopy(sig, 0, tmp, 4, 20);
    tmp[24]=(byte)0x02; tmp[25]=(byte)0x14;
    System.arraycopy(sig, 20, tmp, 26, 20); sig=tmp;
*/  
    return signature.verify(sig);
  }
}
