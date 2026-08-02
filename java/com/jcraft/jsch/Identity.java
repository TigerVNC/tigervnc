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

public interface Identity {

  /**
   * Decrypts this identity with the specified pass-phrase.
   *
   * @param passphrase the pass-phrase for this identity.
   * @return <code>true</code> if the decryption is succeeded or this identity is not cyphered.
   */
  public boolean setPassphrase(byte[] passphrase) throws JSchException;

  /**
   * Returns the public-key blob.
   *
   * @return the public-key blob
   */
  public byte[] getPublicKeyBlob();

  /**
   * Signs on data with this identity, and returns the result.
   *
   * <p>
   * <em>IMPORTANT NOTE:</em> <br>
   * The {@link #getSignature(byte[], String)} method should be overridden to ensure {@code ssh-rsa}
   * type public keys function with the {@code rsa-sha2-256} or {@code rsa-sha2-512} signature
   * algorithms.
   *
   * @param data data to be signed
   * @return the signature
   * @see #getSignature(byte[], String)
   */
  public byte[] getSignature(byte[] data);

  /**
   * Signs on data with this identity, and returns the result.
   *
   * <p>
   * <em>IMPORTANT NOTE:</em> <br>
   * The default implementation of this method simply calls {@link #getSignature(byte[])}, which
   * will fail with {@code ssh-rsa} type public keys when utilized with the {@code rsa-sha2-256} or
   * {@code rsa-sha2-512} signature algorithms: <br>
   * it exists only to maintain backwards compatibility of this interface.
   *
   * <p>
   * This default method should be overridden by implementations to ensure the {@code rsa-sha2-256}
   * and {@code rsa-sha2-512} signature algorithms function correctly.
   *
   * @param data data to be signed
   * @param alg signature algorithm to use
   * @return the signature
   * @since 0.1.57
   * @see #getSignature(byte[])
   */
  public default byte[] getSignature(byte[] data, String alg) {
    return getSignature(data);
  }

  /**
   * This method is deprecated and the default implmentation of this method will throw an
   * {@link UnsupportedOperationException}.
   *
   * @deprecated The decryption should be done automatically in {@link #setPassphrase(byte[])}
   * @return <code>true</code> if the decryption is succeeded or this identity is not cyphered.
   * @see #setPassphrase(byte[])
   */
  @Deprecated
  public default boolean decrypt() {
    throw new UnsupportedOperationException("not implemented");
  }

  /**
   * Returns the name of the key algorithm.
   *
   * @return the name of the key algorithm
   */
  public String getAlgName();

  /**
   * Returns the name of this identity. It will be useful to identify this object in the
   * {@link IdentityRepository}.
   *
   * @return the name of this identity
   */
  public String getName();

  /**
   * Returns <code>true</code> if this identity is cyphered.
   *
   * @return <code>true</code> if this identity is cyphered.
   */
  public boolean isEncrypted();

  /** Disposes internally allocated data, like byte array for the private key. */
  public void clear();
}
