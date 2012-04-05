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

import java.security.SecureRandom;

public class Random implements com.jcraft.jsch.Random{
  private byte[] tmp=new byte[16];
  private SecureRandom random=null;
  public Random(){

    // We hope that 'new SecureRandom()' will use NativePRNG algorithm
    // on Sun's Java5 for GNU/Linux and Solaris.
    // It seems NativePRNG refers to /dev/urandom and it must not be blocked,
    // but NativePRNG is slower than SHA1PRNG ;-<
    // TIPS: By adding option '-Djava.security.egd=file:/dev/./urandom'
    //       SHA1PRNG will be used instead of NativePRNG.
    // On MacOSX, 'new SecureRandom()' will use NativePRNG algorithm and
    // it is also slower than SHA1PRNG.
    // On Windows, 'new SecureRandom()' will use SHA1PRNG algorithm.
    random=new SecureRandom();

    /*
    try{ 
      random=SecureRandom.getInstance("SHA1PRNG"); 
      return;
    }
    catch(java.security.NoSuchAlgorithmException e){ 
      // System.err.println(e); 
    }

    // The following code is for IBM's JCE
    try{ 
      random=SecureRandom.getInstance("IBMSecureRandom"); 
      return;
    }
    catch(java.security.NoSuchAlgorithmException ee){ 
      //System.err.println(ee); 
    }
    */
  }
  public void fill(byte[] foo, int start, int len){
    /*
    // This case will not become true in our usage.
    if(start==0 && foo.length==len){
      random.nextBytes(foo);
      return;
    }
    */
    if(len>tmp.length){ tmp=new byte[len]; }
    random.nextBytes(tmp);
    System.arraycopy(tmp, 0, foo, start, len);
  }
}
