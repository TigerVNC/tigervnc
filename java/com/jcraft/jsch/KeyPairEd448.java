/*
 * Copyright (c) 2002-2018 ymnk, JCraft,Inc. All rights reserved.
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

import java.util.Arrays;

class KeyPairEd448 extends KeyPairEdDSA {

  private static int keySize = 57;

  KeyPairEd448(JSch.InstanceLogger instLogger) {
    this(instLogger, null, null);
  }

  KeyPairEd448(JSch.InstanceLogger instLogger, byte[] pub_array, byte[] prv_array) {
    super(instLogger, pub_array, prv_array);
  }

  @Override
  public int getKeyType() {
    return ED448;
  }

  @Override
  public int getKeySize() {
    return keySize;
  }

  @Override
  String getSshName() {
    return "ssh-ed448";
  }

  @Override
  String getJceName() {
    return "Ed448";
  }

  static KeyPair fromSSHAgent(JSch.InstanceLogger instLogger, Buffer buf) throws JSchException {

    byte[][] tmp = buf.getBytes(4, "invalid key format");

    byte[] pub_array = tmp[1];
    byte[] prv_array = Arrays.copyOf(tmp[2], keySize);
    KeyPairEd448 kpair = new KeyPairEd448(instLogger, pub_array, prv_array);
    kpair.publicKeyComment = Util.byte2str(tmp[3]);
    kpair.vendor = VENDOR_OPENSSH;
    return kpair;
  }
}
