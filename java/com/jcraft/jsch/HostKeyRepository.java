/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2004-2015 ymnk, JCraft,Inc. All rights reserved.

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

public interface HostKeyRepository{
  final int OK=0;
  final int NOT_INCLUDED=1;
  final int CHANGED=2;

  /**
   * Checks if <code>host</code> is included with the <code>key</code>. 
   * 
   * @return #NOT_INCLUDED, #OK or #CHANGED
   * @see #NOT_INCLUDED
   * @see #OK
   * @see #CHANGED
   */
  int check(String host, byte[] key);

  /**
   * Adds a host key <code>hostkey</code>
   *
   * @param hostkey a host key to be added
   * @param ui a user interface for showing messages or promping inputs.
   * @see UserInfo
   */
  void add(HostKey hostkey, UserInfo ui);

  /**
   * Removes a host key if there exists mached key with
   * <code>host</code>, <code>type</code>.
   *
   * @see #remove(String host, String type, byte[] key)
   */
  void remove(String host, String type);

  /**
   * Removes a host key if there exists a matched key with
   * <code>host</code>, <code>type</code> and <code>key</code>.
   */
  void remove(String host, String type, byte[] key);

  /**
   * Returns id of this repository.
   *
   * @return identity in String
   */
  String getKnownHostsRepositoryID();

  /**
   * Retuns a list for host keys managed in this repository.
   *
   * @see #getHostKey(String host, String type)
   */
  HostKey[] getHostKey();

  /**
   * Retuns a list for host keys managed in this repository.
   *
   * @param host a hostname used in searching host keys.
   *        If <code>null</code> is given, every host key will be listed.
   * @param type a key type used in searching host keys,
   *        and it should be "ssh-dss" or "ssh-rsa".
   *        If <code>null</code> is given, a key type type will not be ignored.
   */
  HostKey[] getHostKey(String host, String type);
}
