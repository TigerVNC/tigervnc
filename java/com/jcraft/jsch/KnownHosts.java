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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

class KnownHosts implements HostKeyRepository {
  private JSch jsch = null;
  private String known_hosts = null;
  private Vector<HostKey> pool = null;

  MAC hmacsha1;

  KnownHosts(JSch jsch) {
    super();
    this.jsch = jsch;
    getHMACSHA1();
    pool = new Vector<>();
  }

  void setKnownHosts(String filename) throws JSchException {
    try {
      known_hosts = filename;
      InputStream fis = new FileInputStream(Util.checkTilde(filename));
      setKnownHosts(fis);
    } catch (FileNotFoundException e) {
      // The non-existing file should be allowed.
    }
  }

  void setKnownHosts(InputStream input) throws JSchException {
    pool.removeAllElements();
    StringBuilder sb = new StringBuilder();
    byte i;
    int j;
    boolean error = false;
    try (InputStream fis = input) {
      String host;
      String key = null;
      int type;
      byte[] buf = new byte[1024];
      int bufl = 0;
      loop: while (true) {
        bufl = 0;
        while (true) {
          j = fis.read();
          if (j == -1) {
            if (bufl == 0) {
              break loop;
            }
            break;
          }
          if (j == 0x0d) {
            continue;
          }
          if (j == 0x0a) {
            break;
          }
          if (buf.length <= bufl) {
            if (bufl > 1024 * 10)
              break; // too long...
            byte[] newbuf = new byte[buf.length * 2];
            System.arraycopy(buf, 0, newbuf, 0, buf.length);
            buf = newbuf;
          }
          buf[bufl++] = (byte) j;
        }

        j = 0;
        while (j < bufl) {
          i = buf[j];
          if (i == ' ' || i == '\t') {
            j++;
            continue;
          }
          if (i == '#') {
            addInvalidLine(Util.byte2str(buf, 0, bufl));
            continue loop;
          }
          break;
        }
        if (j >= bufl) {
          addInvalidLine(Util.byte2str(buf, 0, bufl));
          continue loop;
        }

        sb.setLength(0);
        while (j < bufl) {
          i = buf[j++];
          if (i == 0x20 || i == '\t') {
            break;
          }
          sb.append((char) i);
        }
        host = sb.toString();
        if (j >= bufl || host.length() == 0) {
          addInvalidLine(Util.byte2str(buf, 0, bufl));
          continue loop;
        }

        while (j < bufl) {
          i = buf[j];
          if (i == ' ' || i == '\t') {
            j++;
            continue;
          }
          break;
        }

        String marker = "";
        if (host.charAt(0) == '@') {
          marker = host;

          sb.setLength(0);
          while (j < bufl) {
            i = buf[j++];
            if (i == 0x20 || i == '\t') {
              break;
            }
            sb.append((char) i);
          }
          host = sb.toString();
          if (j >= bufl || host.length() == 0) {
            addInvalidLine(Util.byte2str(buf, 0, bufl));
            continue loop;
          }

          while (j < bufl) {
            i = buf[j];
            if (i == ' ' || i == '\t') {
              j++;
              continue;
            }
            break;
          }
        }

        sb.setLength(0);
        while (j < bufl) {
          i = buf[j++];
          if (i == 0x20 || i == '\t') {
            break;
          }
          sb.append((char) i);
        }
        String keyType = sb.toString();
        boolean revokedCertificate =
            "@revoked".equals(marker) && OpenSshCertificateKeyTypes.isCertificateKeyType(keyType);
        type = HostKey.name2type(keyType);
        if (type == HostKey.UNKNOWN && !revokedCertificate) {
          j = bufl;
        }
        if (j >= bufl) {
          addInvalidLine(Util.byte2str(buf, 0, bufl));
          continue loop;
        }

        while (j < bufl) {
          i = buf[j];
          if (i == ' ' || i == '\t') {
            j++;
            continue;
          }
          break;
        }

        sb.setLength(0);
        while (j < bufl) {
          i = buf[j++];
          if (i == 0x0d) {
            continue;
          }
          if (i == 0x0a) {
            break;
          }
          if (i == 0x20 || i == '\t') {
            break;
          }
          sb.append((char) i);
        }
        key = sb.toString();
        if (key.length() == 0) {
          addInvalidLine(Util.byte2str(buf, 0, bufl));
          continue loop;
        }

        while (j < bufl) {
          i = buf[j];
          if (i == ' ' || i == '\t') {
            j++;
            continue;
          }
          break;
        }

        /**
         * "man sshd" has following descriptions, Note that the lines in these files are typically
         * hundreds of characters long, and you definitely don't want to type in the host keys by
         * hand. Rather, generate them by a script, ssh-keyscan(1) or by taking
         * /usr/local/etc/ssh_host_key.pub and adding the host names at the front. This means that a
         * comment is allowed to appear at the end of each key entry.
         */
        String comment = null;
        if (j < bufl) {
          sb.setLength(0);
          while (j < bufl) {
            i = buf[j++];
            if (i == 0x0d) {
              continue;
            }
            if (i == 0x0a) {
              break;
            }
            sb.append((char) i);
          }
          comment = sb.toString();
        }

        // System.err.println(host);
        // System.err.println("|"+key+"|");

        if (revokedCertificate) {
          addRevokedCertificateHostKey(Util.byte2str(buf, 0, bufl), marker, host, keyType, key,
              comment);
          continue loop;
        }

        byte[] keyData = Util.fromBase64(Util.str2byte(key), 0, key.length());
        pool.addElement(new HashedHostKey(marker, host, type, keyData, comment));
      }
      if (error) {
        throw new JSchException("KnownHosts: invalid format");
      }
    } catch (Exception e) {
      if (e instanceof JSchException)
        throw (JSchException) e;
      throw new JSchException(e.toString(), e);
    }
  }

  private void addInvalidLine(String line) throws JSchException {
    HostKey hk = new HostKey(line, HostKey.UNKNOWN, null);
    pool.addElement(hk);
  }

  // Unnamed catch variables require Java 22, but this source set targets Java 8.
  @SuppressWarnings("java:S7467")
  private void addRevokedCertificateHostKey(String line, String marker, String host, String keyType,
      String key, String comment) throws JSchException {
    byte[] keyData;
    int type;
    try {
      OpenSshCertificate certificate = OpenSshCertificateParser.parse(jsch.instLogger,
          Util.fromBase64(Util.str2byte(key), 0, key.length()));
      keyData = certificate.getCertificatePublicKey();
      type = HostKey.name2type(Util.byte2str(new Buffer(keyData).getString()));
      if (type == HostKey.UNKNOWN || type != getCertificateBaseType(keyType)) {
        throw new JSchException("Certificate key type does not match known_hosts key type");
      }
    } catch (JSchException | RuntimeException ignored) {
      addInvalidLine(line);
      return;
    }

    pool.addElement(new HashedHostKey(marker, host, type, keyData, comment));
  }

  private static int getCertificateBaseType(String keyType) {
    String baseType = OpenSshCertificateKeyTypes.getBaseKeyType(keyType);
    if ("rsa-sha2-256".equals(baseType) || "rsa-sha2-512".equals(baseType)) {
      return HostKey.SSHRSA;
    }
    return HostKey.name2type(baseType);
  }

  String getKnownHostsFile() {
    return known_hosts;
  }

  @Override
  public String getKnownHostsRepositoryID() {
    return known_hosts;
  }

  @Override
  public int check(String host, byte[] key) {
    int result = NOT_INCLUDED;
    if (host == null) {
      return result;
    }

    HostKey hk = null;
    try {
      hk = new HostKey(host, HostKey.GUESS, key);
    } catch (Exception e) { // unsupported key
      jsch.getInstanceLogger().log(Logger.DEBUG,
          "exception while trying to read key while checking host '" + host + "'", e);
      return result;
    }

    synchronized (pool) {
      for (int i = 0; i < pool.size(); i++) {
        HostKey _hk = pool.elementAt(i);
        String marker = _hk.getMarker();
        if (_hk.isMatched(host) && _hk.type == hk.type
            && ("".equals(marker) || "@revoked".equals(marker))) {
          if (Util.array_equals(_hk.key, key)) {
            return OK;
          }
          if ("".equals(marker)) {
            result = CHANGED;
          }
        }
      }
    }

    if (result == NOT_INCLUDED && host.startsWith("[") && host.indexOf("]:") > 1) {
      return check(host.substring(1, host.indexOf("]:")), key);
    }

    return result;
  }

  @Override
  public void add(HostKey hostkey, UserInfo userinfo) {
    int type = hostkey.type;
    String host = hostkey.getHost();
    // byte[] key=hostkey.key;

    HostKey hk = null;
    synchronized (pool) {
      for (int i = 0; i < pool.size(); i++) {
        hk = pool.elementAt(i);
        if (hk.isMatched(host) && hk.type == type) {
          /*
           * if(Util.array_equals(hk.key, key)){ return; } if(hk.host.equals(host)){ hk.key=key;
           * return; } else{ hk.host=deleteSubString(hk.host, host); break; }
           */
        }
      }
    }

    hk = hostkey;

    pool.addElement(hk);

    syncKnownHostsFile(userinfo);
  }

  void syncKnownHostsFile(UserInfo userinfo) {
    String khFilename = getKnownHostsRepositoryID();
    if (khFilename == null) {
      return;
    }
    boolean doSync = true;
    File goo = new File(Util.checkTilde(khFilename));
    if (!goo.exists()) {
      doSync = false;
      if (userinfo != null) {
        doSync = userinfo
            .promptYesNo(khFilename + " does not exist.\n" + "Are you sure you want to create it?");
        goo = goo.getParentFile();
        if (doSync && goo != null && !goo.exists()) {
          doSync = userinfo.promptYesNo("The parent directory " + goo + " does not exist.\n"
              + "Are you sure you want to create it?");
          if (doSync) {
            if (!goo.mkdirs()) {
              userinfo.showMessage(goo + " has not been created.");
              doSync = false;
            } else {
              userinfo.showMessage(
                  goo + " has been succesfully created.\nPlease check its access permission.");
            }
          }
        }
        if (goo == null)
          doSync = false;
      }
    }
    if (!doSync) {
      return;
    }
    try {
      sync(khFilename);
    } catch (Exception e) {
      jsch.getInstanceLogger().log(Logger.ERROR, "unable to sync known host file " + goo.getPath(),
          e);
    }
  }

  @Override
  public HostKey[] getHostKey() {
    return getHostKey(null, (String) null);
  }

  @Override
  public HostKey[] getHostKey(String host, String type) {
    synchronized (pool) {
      List<HostKey> v = new ArrayList<>();
      for (int i = 0; i < pool.size(); i++) {
        HostKey hk = pool.elementAt(i);
        if (hk.type == HostKey.UNKNOWN)
          continue;
        if (host == null || (hk.isMatched(host) && (type == null || hk.getType().equals(type)))) {
          v.add(hk);
        }
      }
      HostKey[] foo = new HostKey[v.size()];
      for (int i = 0; i < v.size(); i++) {
        foo[i] = v.get(i);
      }
      if (host != null && host.startsWith("[") && host.indexOf("]:") > 1) {
        HostKey[] tmp = getHostKey(host.substring(1, host.indexOf("]:")), type);
        if (tmp.length > 0) {
          HostKey[] bar = new HostKey[foo.length + tmp.length];
          System.arraycopy(foo, 0, bar, 0, foo.length);
          System.arraycopy(tmp, 0, bar, foo.length, tmp.length);
          foo = bar;
        }
      }
      return foo;
    }
  }

  @Override
  public void remove(String host, String type) {
    remove(host, type, null);
  }

  @Override
  public void remove(String host, String type, byte[] key) {
    boolean sync = false;
    synchronized (pool) {
      for (int i = 0; i < pool.size(); i++) {
        HostKey hk = pool.elementAt(i);
        boolean preserveMarkedEntry =
            host != null && type != null && key == null && !"".equals(hk.getMarker());
        if (!preserveMarkedEntry && (host == null || (hk.isMatched(host) && (type == null
            || (hk.getType().equals(type) && (key == null || Util.array_equals(key, hk.key))))))) {
          String hosts = hk.getHost();
          if (host == null || hosts.equals(host)
              || ((hk instanceof HashedHostKey) && ((HashedHostKey) hk).isHashed())) {
            pool.removeElement(hk);
            i--;
          } else {
            hk.host = deleteSubString(hosts, host);
          }
          sync = true;
        }
      }
    }
    if (sync) {
      try {
        sync();
      } catch (Exception e) {
      } ;
    }
  }

  void sync() throws IOException {
    if (known_hosts != null)
      sync(known_hosts);
  }

  synchronized void sync(String foo) throws IOException {
    if (foo == null)
      return;
    try (FileOutputStream fos = new FileOutputStream(Util.checkTilde(foo))) {
      dump(fos);
    }
  }

  private static final byte[] space = {(byte) 0x20};
  private static final byte[] lf = Util.str2byte("\n");

  void dump(OutputStream out) {
    try {
      HostKey hk;
      synchronized (pool) {
        for (int i = 0; i < pool.size(); i++) {
          hk = pool.elementAt(i);
          dumpHostKey(out, hk);
        }
      }
    } catch (Exception e) {
      jsch.getInstanceLogger().log(Logger.ERROR, "unable to dump known hosts", e);
    }
  }

  void dumpHostKey(OutputStream out, HostKey hk) throws IOException {
    String marker = hk.getMarker();
    String host = hk.getHost();
    String type = hk.getType();
    String comment = hk.getComment();
    if (type.equals("UNKNOWN")) {
      out.write(Util.str2byte(host));
      out.write(lf);
      return;
    }
    if (marker.length() != 0) {
      out.write(Util.str2byte(marker));
      out.write(space);
    }
    out.write(Util.str2byte(host));
    out.write(space);
    out.write(Util.str2byte(type));
    out.write(space);
    out.write(Util.str2byte(hk.getKey()));

    if (comment != null) {
      out.write(space);
      out.write(Util.str2byte(comment));
    }
    out.write(lf);
  }

  String deleteSubString(String hosts, String host) {
    int i = 0;
    int hostlen = host.length();
    int hostslen = hosts.length();
    int j;
    while (i < hostslen) {
      j = hosts.indexOf(',', i);
      if (j == -1)
        break;
      if (!host.equals(hosts.substring(i, j))) {
        i = j + 1;
        continue;
      }
      return hosts.substring(0, i) + hosts.substring(j + 1);
    }
    if (hosts.endsWith(host) && hostslen - i == hostlen) {
      return hosts.substring(0, (hostlen == hostslen) ? 0 : hostslen - hostlen - 1);
    }
    return hosts;
  }

  MAC getHMACSHA1() throws IllegalArgumentException {
    if (hmacsha1 == null) {
      hmacsha1 = createHMAC(JSch.getConfig("hmac-sha1"));
    }

    return hmacsha1;
  }

  MAC createHMAC(String hmacClassname) throws IllegalArgumentException {
    try {
      Class<? extends MAC> c = Class.forName(hmacClassname).asSubclass(MAC.class);
      return c.getDeclaredConstructor().newInstance();
    } catch (Exception e) {
      jsch.getInstanceLogger().log(Logger.ERROR,
          "unable to instantiate HMAC-class " + hmacClassname, e);
      throw new IllegalArgumentException("instantiation of " + hmacClassname + " lead to an error",
          e);
    }
  }

  HostKey createHashedHostKey(String host, byte[] key) throws JSchException {
    HashedHostKey hhk = new HashedHostKey(host, key);
    hhk.hash();
    return hhk;
  }

  class HashedHostKey extends HostKey {
    private static final String HASH_MAGIC = "|1|";
    private static final String HASH_DELIM = "|";

    private boolean hashed = false;
    byte[] salt = null;
    byte[] hash = null;

    HashedHostKey(String host, byte[] key) throws JSchException {
      this(host, GUESS, key);
    }

    HashedHostKey(String host, int type, byte[] key) throws JSchException {
      this("", host, type, key, null);
    }

    HashedHostKey(String marker, String host, int type, byte[] key, String comment)
        throws JSchException {
      super(marker, host, type, key, comment);
      if (this.host.startsWith(HASH_MAGIC)
          && this.host.substring(HASH_MAGIC.length()).indexOf(HASH_DELIM) > 0) {
        String data = this.host.substring(HASH_MAGIC.length());
        String _salt = data.substring(0, data.indexOf(HASH_DELIM));
        String _hash = data.substring(data.indexOf(HASH_DELIM) + 1);
        salt = Util.fromBase64(Util.str2byte(_salt), 0, _salt.length());
        hash = Util.fromBase64(Util.str2byte(_hash), 0, _hash.length());
        int blockSize = hmacsha1.getBlockSize();
        if (salt.length != blockSize || hash.length != blockSize) {
          salt = null;
          hash = null;
          return;
        }
        hashed = true;
      }
    }

    @Override
    boolean isMatched(String _host) {
      if (!hashed) {
        return super.isMatched(_host);
      }
      try {
        synchronized (hmacsha1) {
          hmacsha1.init(salt);
          byte[] foo = Util.str2byte(_host);
          hmacsha1.update(foo, 0, foo.length);
          byte[] bar = new byte[hmacsha1.getBlockSize()];
          hmacsha1.doFinal(bar, 0);
          return Util.array_equals(hash, bar);
        }
      } catch (Exception e) {
        jsch.getInstanceLogger().log(Logger.ERROR,
            "an error occurred while trying to check hash for host " + _host, e);
      }
      return false;
    }

    boolean isHashed() {
      return hashed;
    }

    void hash() {
      if (hashed)
        return;
      if (salt == null) {
        Random random = Session.random;
        synchronized (random) {
          salt = new byte[hmacsha1.getBlockSize()];
          random.fill(salt, 0, salt.length);
        }
      }
      try {
        synchronized (hmacsha1) {
          hmacsha1.init(salt);
          byte[] foo = Util.str2byte(host);
          hmacsha1.update(foo, 0, foo.length);
          hash = new byte[hmacsha1.getBlockSize()];
          hmacsha1.doFinal(hash, 0);
        }
      } catch (Exception e) {
        jsch.getInstanceLogger().log(Logger.ERROR,
            "an error occurred while trying to calculate the hash for host " + host, e);
        salt = null;
        hash = null;
        return;
      }
      host = HASH_MAGIC + Util.byte2str(Util.toBase64(salt, 0, salt.length, true)) + HASH_DELIM
          + Util.byte2str(Util.toBase64(hash, 0, hash.length, true));
      hashed = true;
    }
  }
}
