/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2012 ymnk, JCraft,Inc. All rights reserved.

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

import java.util.Vector;

class LocalIdentityRepository implements IdentityRepository {

  private Vector identities = new Vector();
  private JSch jsch;

  LocalIdentityRepository(JSch jsch){
    this.jsch = jsch;
  }

  public synchronized Vector getIdentities() {
    Vector v = new Vector();
    for(int i=0; i<identities.size(); i++){
      v.addElement(identities.elementAt(i));
    }
    return v;
  }

  public synchronized void add(Identity identity) {
    if(!identities.contains(identity)) {
      identities.addElement(identity);
    }
  }

  public synchronized boolean add(byte[] identity) {
    try{
      Identity _identity =
        IdentityFile.newInstance("from remote:", identity, null, jsch);
      identities.addElement(_identity);
      return true;
    }
    catch(JSchException e){
      return false;
    }
  }

  public synchronized boolean remove(byte[] blob) {
    if(blob == null) return false;
    for(int i=0; i<identities.size(); i++) {
      Identity _identity = (Identity)(identities.elementAt(i));
      byte[] _blob = _identity.getPublicKeyBlob();
      if(_blob == null || !Util.array_equals(blob, _blob))
        continue;
      identities.removeElement(_identity);
      _identity.clear();
      return true;
    }
    return false;
  }

  public synchronized void removeAll() {
    for(int i=0; i<identities.size(); i++) {
      Identity identity=(Identity)(identities.elementAt(i));
      identity.clear();
    }
    identities.removeAllElements();
  } 
}
