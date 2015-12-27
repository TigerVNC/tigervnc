/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2006-2015 ymnk, JCraft,Inc. All rights reserved.

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

package com.jcraft.jsch.jcraft;

import java.security.*;

class HMAC{

  /*
   * Refer to RFC2104.
   *
   * H(K XOR opad, H(K XOR ipad, text))
   *
   * where K is an n byte key
   * ipad is the byte 0x36 repeated 64 times
   * opad is the byte 0x5c repeated 64 times
   * and text is the data being protected
   */
  private static final int B=64;
  private byte[] k_ipad=null;
  private byte[] k_opad=null;

  private MessageDigest md=null;

  private int bsize=0;

  protected void setH(MessageDigest md){
    this.md=md;
    bsize=md.getDigestLength();
  }

  public int getBlockSize(){return bsize;};
  public void init(byte[] key) throws Exception{
    md.reset();
    if(key.length>bsize){
      byte[] tmp=new byte[bsize];
      System.arraycopy(key, 0, tmp, 0, bsize);	  
      key=tmp;
    }

    /* if key is longer than B bytes reset it to key=MD5(key) */
    if(key.length>B){
      md.update(key, 0, key.length);
      key=md.digest();
    }

    k_ipad=new byte[B];
    System.arraycopy(key, 0, k_ipad, 0, key.length);
    k_opad=new byte[B];
    System.arraycopy(key, 0, k_opad, 0, key.length);

    /* XOR key with ipad and opad values */
    for(int i=0; i<B; i++) {
      k_ipad[i]^=(byte)0x36;
      k_opad[i]^=(byte)0x5c;
    }

    md.update(k_ipad, 0, B);
  }

  private final byte[] tmp=new byte[4];
  public void update(int i){
    tmp[0]=(byte)(i>>>24);
    tmp[1]=(byte)(i>>>16);
    tmp[2]=(byte)(i>>>8);
    tmp[3]=(byte)i;
    update(tmp, 0, 4);
  }

  public void update(byte foo[], int s, int l){
    md.update(foo, s, l);
  }

  public void doFinal(byte[] buf, int offset){
    byte[] result=md.digest();
    md.update(k_opad, 0, B);
    md.update(result, 0, bsize);
    try{md.digest(buf, offset, bsize);}catch(Exception e){}
    md.update(k_ipad, 0, B);
  }
}
