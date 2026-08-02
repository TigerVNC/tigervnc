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

import java.util.Locale;

public class HostKey {

  private static final byte[][] names =
      {Util.str2byte("ssh-dss"), Util.str2byte("ssh-rsa"), Util.str2byte("ecdsa-sha2-nistp256"),
          Util.str2byte("ecdsa-sha2-nistp384"), Util.str2byte("ecdsa-sha2-nistp521"),
          Util.str2byte("ssh-ed25519"), Util.str2byte("ssh-ed448")};

  public static final int UNKNOWN = -1;
  public static final int GUESS = 0;
  public static final int SSHDSS = 1;
  public static final int SSHRSA = 2;
  public static final int ECDSA256 = 3;
  public static final int ECDSA384 = 4;
  public static final int ECDSA521 = 5;
  public static final int ED25519 = 6;
  public static final int ED448 = 7;

  protected String marker;
  protected String host;
  protected int type;
  protected byte[] key;
  protected String comment;

  public HostKey(String host, byte[] key) throws JSchException {
    this(host, GUESS, key);
  }

  public HostKey(String host, int type, byte[] key) throws JSchException {
    this(host, type, key, null);
  }

  public HostKey(String host, int type, byte[] key, String comment) throws JSchException {
    this("", host, type, key, comment);
  }

  public HostKey(String marker, String host, int type, byte[] key, String comment)
      throws JSchException {
    this.marker = marker == null ? "" : marker;
    this.host = host;
    if (type == GUESS) {
      if (key[8] == 'd') {
        this.type = SSHDSS;
      } else if (key[8] == 'r') {
        this.type = SSHRSA;
      } else if (key[8] == 'e' && key[10] == '2') {
        this.type = ED25519;
      } else if (key[8] == 'e' && key[10] == '4') {
        this.type = ED448;
      } else if (key[8] == 'a' && key[20] == '2') {
        this.type = ECDSA256;
      } else if (key[8] == 'a' && key[20] == '3') {
        this.type = ECDSA384;
      } else if (key[8] == 'a' && key[20] == '5') {
        this.type = ECDSA521;
      } else {
        throw new JSchException("invalid key type");
      }
    } else {
      this.type = type;
    }
    this.key = key;
    this.comment = comment;
  }

  public String getHost() {
    return host;
  }

  public String getType() {
    if (type == SSHDSS || type == SSHRSA || type == ED25519 || type == ED448 || type == ECDSA256
        || type == ECDSA384 || type == ECDSA521) {
      return Util.byte2str(names[type - 1]);
    }
    return "UNKNOWN";
  }

  protected static int name2type(String name) {
    for (int i = 0; i < names.length; i++) {
      if (Util.byte2str(names[i]).equals(name)) {
        return i + 1;
      }
    }
    return UNKNOWN;
  }

  public String getKey() {
    return Util.byte2str(Util.toBase64(key, 0, key.length, true));
  }

  public String getFingerPrint(JSch jsch) {
    HASH hash = null;
    try {
      String _c = JSch.getConfig("FingerprintHash").toLowerCase(Locale.ROOT);
      Class<? extends HASH> c = Class.forName(JSch.getConfig(_c)).asSubclass(HASH.class);
      hash = c.getDeclaredConstructor().newInstance();
    } catch (Exception e) {
      if (jsch.getInstanceLogger().isEnabled(Logger.ERROR)) {
        jsch.getInstanceLogger().log(Logger.ERROR, "getFingerPrint: " + e.getMessage(), e);
      }
    }
    return Util.getFingerPrint(hash, key, true, false);
  }

  public String getComment() {
    return comment;
  }

  public String getMarker() {
    return marker;
  }

  boolean isMatched(String _host) {
    return isIncluded(_host);
  }

  /**
   * Checks if the given hostname matches any of the host patterns in this HostKey, supporting
   * OpenSSH-style wildcards.
   * <p>
   * This method supports wildcard patterns similar to OpenSSH's known_hosts file:
   * </p>
   * <ul>
   * <li>{@code *} - Matches zero or more characters</li>
   * <li>{@code ?} - Matches exactly one character</li>
   * </ul>
   * <p>
   * The host field can contain multiple comma-separated patterns. The method returns {@code true}
   * if the hostname matches ANY of the patterns.
   * </p>
   * <p>
   * Examples:
   * </p>
   * <ul>
   * <li>{@code *.example.com} matches {@code host.example.com}, {@code sub.example.com}</li>
   * <li>{@code host?.example.com} matches {@code host1.example.com}, {@code hosta.example.com}</li>
   * <li>{@code 192.168.1.*} matches {@code 192.168.1.1}, {@code 192.168.1.100}</li>
   * <li>{@code host1.com,*.host2.com} matches {@code host1.com} or any subdomain of
   * {@code host2.com}</li>
   * </ul>
   *
   * @param _host the hostname to test against the patterns, must not be {@code null}
   * @return {@code true} if the hostname matches any of the patterns (with wildcard support);
   *         {@code false} otherwise
   * @see #isMatched(String)
   */
  boolean isWildcardMatched(String _host) {
    if (_host == null) {
      return false;
    }

    String hosts = this.host;
    if (hosts == null || hosts.isEmpty()) {
      return false;
    }

    // Split by comma and check each pattern
    int i = 0;
    int hostslen = hosts.length();
    while (i < hostslen) {
      int j = hosts.indexOf(',', i);
      String pattern;
      if (j == -1) {
        pattern = hosts.substring(i).trim();
        if (matchesWildcardPattern(pattern, _host)) {
          return true;
        }
        break;
      } else {
        pattern = hosts.substring(i, j).trim();
        if (matchesWildcardPattern(pattern, _host)) {
          return true;
        }
        i = j + 1;
      }
    }
    return false;
  }

  /**
   * Tests if a hostname matches a single wildcard pattern.
   * <p>
   * This method implements wildcard matching similar to OpenSSH, supporting:
   * </p>
   * <ul>
   * <li>{@code *} - Matches zero or more characters</li>
   * <li>{@code ?} - Matches exactly one character</li>
   * </ul>
   * <p>
   * The matching is case-sensitive to maintain consistency with OpenSSH behavior.
   * </p>
   *
   * @param pattern the wildcard pattern to match against (e.g., {@code *.example.com})
   * @param hostname the hostname to test (e.g., {@code host.example.com})
   * @return {@code true} if the hostname matches the pattern; {@code false} otherwise
   */
  private boolean matchesWildcardPattern(String pattern, String hostname) {
    if (pattern == null || hostname == null) {
      return false;
    }

    int pLen = pattern.length();
    int hLen = hostname.length();
    int p = 0; // pattern index
    int h = 0; // hostname index
    int starIdx = -1; // last '*' position in pattern
    int matchIdx = 0; // position in hostname after last '*' match

    while (h < hLen) {
      if (p < pLen && (pattern.charAt(p) == '?' || pattern.charAt(p) == hostname.charAt(h))) {
        // Match single character or '?'
        p++;
        h++;
      } else if (p < pLen && pattern.charAt(p) == '*') {
        // Found '*', record position and try to match rest
        starIdx = p;
        matchIdx = h;
        p++;
      } else if (starIdx != -1) {
        // No match, but we have a previous '*', backtrack
        p = starIdx + 1;
        matchIdx++;
        h = matchIdx;
      } else {
        // No match and no '*' to backtrack to
        return false;
      }
    }

    // Process remaining pattern characters (should be all '*')
    while (p < pLen && pattern.charAt(p) == '*') {
      p++;
    }

    // Match if we've consumed entire pattern
    return p == pLen;
  }

  private boolean isIncluded(String _host) {
    int i = 0;
    String hosts = this.host;
    int hostslen = hosts.length();
    int hostlen = _host.length();
    int j;
    while (i < hostslen) {
      j = hosts.indexOf(',', i);
      if (j == -1) {
        if (hostlen != hostslen - i)
          return false;
        return hosts.regionMatches(true, i, _host, 0, hostlen);
      }
      if (hostlen == (j - i)) {
        if (hosts.regionMatches(true, i, _host, 0, hostlen))
          return true;
      }
      i = j + 1;
    }
    return false;
  }
}
