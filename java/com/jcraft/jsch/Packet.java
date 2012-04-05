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

package com.jcraft.jsch;

public class Packet{

  private static Random random=null;
  static void setRandom(Random foo){ random=foo;}

  Buffer buffer;
  byte[] ba4=new byte[4]; 
  public Packet(Buffer buffer){
    this.buffer=buffer;
  }
  public void reset(){
    buffer.index=5;
  }
  void padding(int bsize){
    int len=buffer.index;
    int pad=(-len)&(bsize-1);
    if(pad<bsize){
      pad+=bsize;
    }
    len=len+pad-4;
    ba4[0]=(byte)(len>>>24);
    ba4[1]=(byte)(len>>>16);
    ba4[2]=(byte)(len>>>8);
    ba4[3]=(byte)(len);
    System.arraycopy(ba4, 0, buffer.buffer, 0, 4);
    buffer.buffer[4]=(byte)pad;
    synchronized(random){
      random.fill(buffer.buffer, buffer.index, pad);
    }
    buffer.skip(pad);
    //buffer.putPad(pad);
/*
for(int i=0; i<buffer.index; i++){
System.err.print(Integer.toHexString(buffer.buffer[i]&0xff)+":");
}
System.err.println("");
*/
  }

  int shift(int len, int bsize, int mac){
    int s=len+5+9;
    int pad=(-s)&(bsize-1);
    if(pad<bsize)pad+=bsize;
    s+=pad;
    s+=mac;
    s+=32; // margin for deflater; deflater may inflate data

    /**/
    if(buffer.buffer.length<s+buffer.index-5-9-len){
      byte[] foo=new byte[s+buffer.index-5-9-len];
      System.arraycopy(buffer.buffer, 0, foo, 0, buffer.buffer.length);
      buffer.buffer=foo;
    }
    /**/

//if(buffer.buffer.length<len+5+9)
//  System.err.println("buffer.buffer.length="+buffer.buffer.length+" len+5+9="+(len+5+9));

//if(buffer.buffer.length<s)
//  System.err.println("buffer.buffer.length="+buffer.buffer.length+" s="+(s));

    System.arraycopy(buffer.buffer, 
		     len+5+9, 
		     buffer.buffer, s, buffer.index-5-9-len);

    buffer.index=10;
    buffer.putInt(len);
    buffer.index=len+5+9;
    return s;
  }
  void unshift(byte command, int recipient, int s, int len){
    System.arraycopy(buffer.buffer, 
		     s, 
		     buffer.buffer, 5+9, len);
    buffer.buffer[5]=command;
    buffer.index=6;
    buffer.putInt(recipient);
    buffer.putInt(len);
    buffer.index=len+5+9;
  }
  Buffer getBuffer(){
    return buffer;
  }
}
