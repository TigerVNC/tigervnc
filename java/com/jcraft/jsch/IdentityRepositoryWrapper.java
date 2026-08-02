/*
 * Copyright (c) 2012-2018 ymnk, JCraft,Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3. The names of the authors may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JCRAFT, INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.jcraft.jsch;

import java.util.Vector;

/**
 * JSch will accept ciphered keys, but some implementations of IdentityRepository can not. For
 * example, IdentityRepository for ssh-agent and pageant only accept plain keys. The following class
 * has been introduced to cache ciphered keys for them, and pass them whenever they are de-ciphered.
 */
class IdentityRepositoryWrapper implements IdentityRepository {
  private IdentityRepository ir;
  private Vector<Identity> cache = new Vector<>();
  private boolean keep_in_cache = false;

  IdentityRepositoryWrapper(IdentityRepository ir) {
    this(ir, false);
  }

  IdentityRepositoryWrapper(IdentityRepository ir, boolean keep_in_cache) {
    this.ir = ir;
    this.keep_in_cache = keep_in_cache;
  }

  @Override
  public String getName() {
    return ir.getName();
  }

  @Override
  public int getStatus() {
    return ir.getStatus();
  }

  @Override
  public boolean add(byte[] identity) {
    return ir.add(identity);
  }

  @Override
  public boolean remove(byte[] blob) {
    return ir.remove(blob);
  }

  @Override
  public void removeAll() {
    cache.removeAllElements();
    ir.removeAll();
  }

  @Override
  public Vector<Identity> getIdentities() {
    Vector<Identity> result = new Vector<>();
    for (int i = 0; i < cache.size(); i++) {
      Identity identity = cache.elementAt(i);
      result.add(identity);
    }
    Vector<Identity> tmp = ir.getIdentities();
    for (int i = 0; i < tmp.size(); i++) {
      result.add(tmp.elementAt(i));
    }
    return result;
  }

  void add(Identity identity) {
    if (!keep_in_cache && !identity.isEncrypted() && (identity instanceof IdentityFile)) {
      try {
        ir.add(((IdentityFile) identity).getKeyPair().forSSHAgent());
      } catch (JSchException e) {
        // an exception will not be thrown.
      }
    } else
      cache.addElement(identity);
  }

  void check() {
    if (cache.size() > 0) {
      Object[] identities = cache.toArray();
      for (int i = 0; i < identities.length; i++) {
        Identity identity = (Identity) (identities[i]);
        cache.removeElement(identity);
        add(identity);
      }
    }
  }
}
