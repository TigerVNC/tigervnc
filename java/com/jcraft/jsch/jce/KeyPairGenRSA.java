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

import java.security.*;
import java.security.interfaces.*;

public class KeyPairGenRSA implements com.jcraft.jsch.KeyPairGenRSA{
  byte[] d;  // private
  byte[] e;  // public
  byte[] n;

  byte[] c; //  coefficient
  byte[] ep; // exponent p
  byte[] eq; // exponent q
  byte[] p;  // prime p
  byte[] q;  // prime q

  public void init(int key_size) throws Exception{
    KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");
    keyGen.initialize(key_size, new SecureRandom());
    KeyPair pair = keyGen.generateKeyPair();

    PublicKey pubKey=pair.getPublic();
    PrivateKey prvKey=pair.getPrivate();

    d=((RSAPrivateKey)prvKey).getPrivateExponent().toByteArray();
    e=((RSAPublicKey)pubKey).getPublicExponent().toByteArray();
    n=((RSAPrivateKey)prvKey).getModulus().toByteArray();

    c=((RSAPrivateCrtKey)prvKey).getCrtCoefficient().toByteArray();
    ep=((RSAPrivateCrtKey)prvKey).getPrimeExponentP().toByteArray();
    eq=((RSAPrivateCrtKey)prvKey).getPrimeExponentQ().toByteArray();
    p=((RSAPrivateCrtKey)prvKey).getPrimeP().toByteArray();
    q=((RSAPrivateCrtKey)prvKey).getPrimeQ().toByteArray();
  }
  public byte[] getD(){return d;}
  public byte[] getE(){return e;}
  public byte[] getN(){return n;}
  public byte[] getC(){return c;}
  public byte[] getEP(){return ep;}
  public byte[] getEQ(){return eq;}
  public byte[] getP(){return p;}
  public byte[] getQ(){return q;}
}
