/*
 * Copyright (c) 2008-2018 ymnk, JCraft,Inc. All rights reserved.
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

package com.jcraft.jsch.jce;

import java.nio.ByteBuffer;
import javax.crypto.Cipher;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

abstract class AESGCM implements com.jcraft.jsch.Cipher {
  // Actually the block size, not IV size
  private static final int ivsize = 16;
  private static final int tagsize = 16;
  private Cipher cipher;
  private SecretKeySpec keyspec;
  private int mode;
  private ByteBuffer iv;
  private long initcounter;

  @Override
  public int getIVSize() {
    return ivsize;
  }

  @Override
  public int getTagSize() {
    return tagsize;
  }

  @Override
  public void init(int mode, byte[] key, byte[] iv) throws Exception {
    byte[] tmp;
    if (iv.length > 12) {
      tmp = new byte[12];
      System.arraycopy(iv, 0, tmp, 0, tmp.length);
      iv = tmp;
    }
    int bsize = getBlockSize();
    if (key.length > bsize) {
      tmp = new byte[bsize];
      System.arraycopy(key, 0, tmp, 0, tmp.length);
      key = tmp;
    }
    this.mode =
        ((mode == com.jcraft.jsch.Cipher.ENCRYPT_MODE) ? Cipher.ENCRYPT_MODE : Cipher.DECRYPT_MODE);
    this.iv = ByteBuffer.wrap(iv);
    this.initcounter = this.iv.getLong(4);
    try {
      keyspec = new SecretKeySpec(key, "AES");
      cipher = Cipher.getInstance("AES/GCM/NoPadding");
      cipher.init(this.mode, keyspec, new GCMParameterSpec(tagsize * 8, iv));
    } catch (Exception e) {
      cipher = null;
      keyspec = null;
      this.iv = null;
      throw e;
    }
  }

  @Override
  public void update(byte[] foo, int s1, int len, byte[] bar, int s2) throws Exception {
    cipher.update(foo, s1, len, bar, s2);
  }

  @Override
  public void updateAAD(byte[] foo, int s1, int len) throws Exception {
    cipher.updateAAD(foo, s1, len);
  }

  @Override
  public void doFinal(byte[] foo, int s1, int len, byte[] bar, int s2) throws Exception {
    cipher.doFinal(foo, s1, len, bar, s2);
    long newcounter = iv.getLong(4) + 1;
    if (newcounter == initcounter) {
      throw new IllegalStateException("GCM IV would be reused");
    }
    iv.putLong(4, newcounter);
    cipher.init(mode, keyspec, new GCMParameterSpec(tagsize * 8, iv.array()));
  }

  @Override
  public boolean isCBC() {
    return false;
  }

  @Override
  public boolean isAEAD() {
    return true;
  }
}
