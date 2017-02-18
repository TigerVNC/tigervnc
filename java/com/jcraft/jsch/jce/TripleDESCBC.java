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

package com.jcraft.jsch.jce;

import com.jcraft.jsch.Cipher;
import javax.crypto.*;
import javax.crypto.spec.*;

public class TripleDESCBC implements Cipher{
  private static final int ivsize=8;
  private static final int bsize=24;
  private javax.crypto.Cipher cipher;    
  public int getIVSize(){return ivsize;} 
  public int getBlockSize(){return bsize;}
  public void init(int mode, byte[] key, byte[] iv) throws Exception{
    String pad="NoPadding";      
    //if(padding) pad="PKCS5Padding";
    byte[] tmp;
    if(iv.length>ivsize){
      tmp=new byte[ivsize];
      System.arraycopy(iv, 0, tmp, 0, tmp.length);
      iv=tmp;
    }
    if(key.length>bsize){
      tmp=new byte[bsize];
      System.arraycopy(key, 0, tmp, 0, tmp.length);
      key=tmp;
    }

    try{
      cipher=javax.crypto.Cipher.getInstance("DESede/CBC/"+pad);
/*
      // The following code does not work on IBM's JDK 1.4.1
      SecretKeySpec skeySpec = new SecretKeySpec(key, "DESede");
      cipher.init((mode==ENCRYPT_MODE?
		   javax.crypto.Cipher.ENCRYPT_MODE:
		   javax.crypto.Cipher.DECRYPT_MODE),
		  skeySpec, new IvParameterSpec(iv));
*/
      DESedeKeySpec keyspec=new DESedeKeySpec(key);
      SecretKeyFactory keyfactory=SecretKeyFactory.getInstance("DESede");
      SecretKey _key=keyfactory.generateSecret(keyspec);
      synchronized(javax.crypto.Cipher.class){
        cipher.init((mode==ENCRYPT_MODE?
                     javax.crypto.Cipher.ENCRYPT_MODE:
                     javax.crypto.Cipher.DECRYPT_MODE),
                    _key, new IvParameterSpec(iv));
      }
    }
    catch(Exception e){
      cipher=null;
      throw e;
    }
  }
  public void update(byte[] foo, int s1, int len, byte[] bar, int s2) throws Exception{
    cipher.update(foo, s1, len, bar, s2);
  }
  public boolean isCBC(){return true; }
}
