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

import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.Properties;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import javax.crypto.AEADBadTagException;

public class Session {

  // http://ietf.org/internet-drafts/draft-ietf-secsh-assignednumbers-01.txt
  static final int SSH_MSG_DISCONNECT = 1;
  static final int SSH_MSG_IGNORE = 2;
  static final int SSH_MSG_UNIMPLEMENTED = 3;
  static final int SSH_MSG_DEBUG = 4;
  static final int SSH_MSG_SERVICE_REQUEST = 5;
  static final int SSH_MSG_SERVICE_ACCEPT = 6;
  static final int SSH_MSG_EXT_INFO = 7;
  static final int SSH_MSG_KEXINIT = 20;
  static final int SSH_MSG_NEWKEYS = 21;
  static final int SSH_MSG_KEXDH_INIT = 30;
  static final int SSH_MSG_KEXDH_REPLY = 31;
  static final int SSH_MSG_KEX_DH_GEX_GROUP = 31;
  static final int SSH_MSG_KEX_DH_GEX_INIT = 32;
  static final int SSH_MSG_KEX_DH_GEX_REPLY = 33;
  static final int SSH_MSG_KEX_DH_GEX_REQUEST = 34;
  static final int SSH_MSG_GLOBAL_REQUEST = 80;
  static final int SSH_MSG_REQUEST_SUCCESS = 81;
  static final int SSH_MSG_REQUEST_FAILURE = 82;
  static final int SSH_MSG_CHANNEL_OPEN = 90;
  static final int SSH_MSG_CHANNEL_OPEN_CONFIRMATION = 91;
  static final int SSH_MSG_CHANNEL_OPEN_FAILURE = 92;
  static final int SSH_MSG_CHANNEL_WINDOW_ADJUST = 93;
  static final int SSH_MSG_CHANNEL_DATA = 94;
  static final int SSH_MSG_CHANNEL_EXTENDED_DATA = 95;
  static final int SSH_MSG_CHANNEL_EOF = 96;
  static final int SSH_MSG_CHANNEL_CLOSE = 97;
  static final int SSH_MSG_CHANNEL_REQUEST = 98;
  static final int SSH_MSG_CHANNEL_SUCCESS = 99;
  static final int SSH_MSG_CHANNEL_FAILURE = 100;

  private static final int PACKET_MAX_SIZE = 256 * 1024;

  private byte[] V_S; // server version
  private byte[] V_C = Util.str2byte("SSH-2.0-JSCH_" + JSch.VERSION); // client version

  private byte[] I_C; // the payload of the client's SSH_MSG_KEXINIT
  byte[] I_S; // the payload of the server's SSH_MSG_KEXINIT
  private byte[] K_S; // the host key

  private byte[] session_id;

  private byte[] IVc2s;
  private byte[] IVs2c;
  private byte[] Ec2s;
  private byte[] Es2c;
  private byte[] MACc2s;
  private byte[] MACs2c;

  // RFC 4253 6.4. each direction must run independently hence we have an incoming and outgoing
  // sequence number
  private int seqi = 0;
  private int seqo = 0;

  String[] guess = null;
  private Cipher s2ccipher;
  private Cipher c2scipher;
  private MAC s2cmac;
  private MAC c2smac;
  // private byte[] mac_buf;
  private byte[] s2cmac_result1;
  private byte[] s2cmac_result2;

  private Compression deflater;
  private Compression inflater;

  private IO io;
  private Socket socket;
  private int timeout = 0;

  private volatile boolean isConnected = false;

  private volatile boolean doExtInfo = false;
  private boolean enable_server_sig_algs = true;
  private boolean enable_ext_info_in_auth = true;

  private volatile boolean initialKex = true;
  private volatile boolean doStrictKex = false;
  private boolean enable_strict_kex = true;
  private boolean require_strict_kex = false;

  private volatile boolean isAuthed = false;

  private Thread connectThread = null;
  private Object lock = new Object();

  boolean x11_forwarding = false;
  boolean agent_forwarding = false;

  InputStream in = null;
  OutputStream out = null;

  static Random random;

  Buffer buf;
  Packet packet;

  SocketFactory socket_factory = null;

  private Hashtable<String, String> config = null;

  private Proxy proxy = null;
  private UserInfo userinfo;

  private String hostKeyAlias = null;
  private int serverAliveInterval = 0;
  private int serverAliveCountMax = 1;

  private IdentityRepository identityRepository = null;
  private HostKeyRepository hostkeyRepository = null;
  private volatile String[] serverSigAlgs = null;
  private volatile boolean sshBugSigType74 = false;

  protected boolean daemon_thread = false;

  private volatile long kex_start_time = 0L;

  int max_auth_tries = 6;
  int auth_failures = 0;

  String host = "127.0.0.1";
  String org_host = "127.0.0.1";
  int port = 22;

  String username = null;
  byte[] password = null;

  JSch jsch;
  Logger logger;

  private ThreadFactory threadFactory = Thread::new;

  private boolean disconnectingChannels = false;
  private final List<Channel> channels = new ArrayList<>();
  private final ReadWriteLock channelsLock = new ReentrantReadWriteLock();

  Session(JSch jsch, String username, String host, int port) throws JSchException {
    super();
    this.jsch = jsch;
    buf = new Buffer();
    packet = new Packet(buf);
    this.username = username;
    this.org_host = this.host = host;
    this.port = port;

    applyConfig();

    if (this.username == null) {
      this.username = Util.getSystemProperty("user.name");
    }

    if (this.username == null) {
      throw new JSchException("username is not given.");
    }
  }

  public void connect() throws JSchException {
    connect(timeout);
  }

  public void connect(int connectTimeout) throws JSchException {
    if (isConnected) {
      throw new JSchException("session is already connected");
    }
    initialKex = true;

    io = new IO();
    if (random == null) {
      try {
        Class<? extends Random> c = Class.forName(getConfig("random")).asSubclass(Random.class);
        random = c.getDeclaredConstructor().newInstance();
      } catch (Exception e) {
        throw new JSchException(e.toString(), e);
      }
    }
    Packet.setRandom(random);

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "Connecting to " + host + " port " + port);
    }

    try {
      int i, j;

      if (proxy == null) {
        InputStream in;
        OutputStream out;
        if (socket_factory == null) {
          socket = Util.createSocket(host, port, connectTimeout);
          in = socket.getInputStream();
          out = socket.getOutputStream();
        } else {
          socket = socket_factory.createSocket(host, port);
          in = socket_factory.getInputStream(socket);
          out = socket_factory.getOutputStream(socket);
        }
        // if(timeout>0){ socket.setSoTimeout(timeout); }
        socket.setTcpNoDelay(true);
        io.setInputStream(in);
        io.setOutputStream(out);
      } else {
        synchronized (proxy) {
          proxy.connect(socket_factory, host, port, connectTimeout);
          io.setInputStream(proxy.getInputStream());
          io.setOutputStream(proxy.getOutputStream());
          socket = proxy.getSocket();
        }
      }

      if (connectTimeout > 0 && socket != null) {
        socket.setSoTimeout(connectTimeout);
      }

      isConnected = true;

      if (getLogger().isEnabled(Logger.INFO)) {
        getLogger().log(Logger.INFO, "Connection established");
      }

      jsch.addSession(this);

      {
        // Some Cisco devices will miss to read '\n' if it is sent separately.
        byte[] foo = new byte[V_C.length + 2];
        System.arraycopy(V_C, 0, foo, 0, V_C.length);
        foo[foo.length - 2] = (byte) '\r';
        foo[foo.length - 1] = (byte) '\n';
        io.put(foo, 0, foo.length);
      }

      while (true) {
        i = 0;
        j = 0;
        while (i < buf.buffer.length) {
          j = io.getByte();
          if (j < 0)
            break;
          buf.buffer[i] = (byte) j;
          i++;
          if (j == 10)
            break;
        }
        if (j < 0) {
          throw new JSchException("connection is closed by foreign host");
        }

        if (buf.buffer[i - 1] == 10) { // 0x0a
          i--;
          if (i > 0 && buf.buffer[i - 1] == 13) { // 0x0d
            i--;
          }
        }

        if (i <= 3 || ((i != buf.buffer.length) && (buf.buffer[0] != 'S' || buf.buffer[1] != 'S'
            || buf.buffer[2] != 'H' || buf.buffer[3] != '-'))) {
          // It must not start with 'SSH-'
          // System.err.println(new String(buf.buffer, 0, i);
          continue;
        }

        if (i == buf.buffer.length || i < 7 || // SSH-1.99 or SSH-2.0
            (buf.buffer[4] == '1' && buf.buffer[6] != '9') // SSH-1.5
        ) {
          throw new JSchException("invalid server's version string");
        }
        break;
      }

      V_S = new byte[i];
      System.arraycopy(buf.buffer, 0, V_S, 0, i);
      // System.err.println("V_S: ("+i+") ["+new String(V_S)+"]");
      String _v_s = Util.byte2str(V_S);
      sshBugSigType74 = _v_s.startsWith("SSH-2.0-OpenSSH_7.4");

      if (getLogger().isEnabled(Logger.INFO)) {
        getLogger().log(Logger.INFO, "Remote version string: " + _v_s);
        getLogger().log(Logger.INFO, "Local version string: " + Util.byte2str(V_C));
      }

      enable_server_sig_algs = getConfig("enable_server_sig_algs").equals("yes");
      enable_ext_info_in_auth = getConfig("enable_ext_info_in_auth").equals("yes");
      enable_strict_kex = getConfig("enable_strict_kex").equals("yes");
      require_strict_kex = getConfig("require_strict_kex").equals("yes");
      send_kexinit();

      buf = read(buf);
      if (buf.getCommand() != SSH_MSG_KEXINIT) {
        in_kex = false;
        throw new JSchException("invalid protocol: " + buf.getCommand());
      }

      if (getLogger().isEnabled(Logger.INFO)) {
        getLogger().log(Logger.INFO, "SSH_MSG_KEXINIT received");
      }

      KeyExchange kex = receive_kexinit(buf);

      while (true) {
        buf = read(buf);
        if (kex.getState() == buf.getCommand()) {
          kex_start_time = System.currentTimeMillis();
          boolean result = kex.next(buf);
          if (!result) {
            // System.err.println("verify: "+result);
            in_kex = false;
            throw new JSchException("verify: " + result);
          }
        } else {
          in_kex = false;
          throw new JSchException("invalid protocol(kex): " + buf.getCommand());
        }
        if (kex.getState() == KeyExchange.STATE_END) {
          break;
        }
      }

      try {
        long tmp = System.currentTimeMillis();
        in_prompt = true;
        checkHost(host, port, kex);
        in_prompt = false;
        kex_start_time += (System.currentTimeMillis() - tmp);
      } catch (JSchException ee) {
        in_kex = false;
        in_prompt = false;
        throw ee;
      }

      send_newkeys();

      // receive SSH_MSG_NEWKEYS(21)
      buf = read(buf);
      // System.err.println("read: 21 ? "+buf.getCommand());
      if (buf.getCommand() == SSH_MSG_NEWKEYS) {

        if (getLogger().isEnabled(Logger.INFO)) {
          getLogger().log(Logger.INFO, "SSH_MSG_NEWKEYS received");
        }

        receive_newkeys(buf, kex);
        initialKex = false;
      } else {
        in_kex = false;
        throw new JSchException("invalid protocol(newkeys): " + buf.getCommand());
      }

      if (enable_server_sig_algs && enable_ext_info_in_auth && doExtInfo) {
        send_extinfo();
      }

      try {
        String s = getConfig("MaxAuthTries");
        if (s != null) {
          max_auth_tries = Integer.parseInt(s);
        }
      } catch (NumberFormatException e) {
        throw new JSchException("MaxAuthTries: " + getConfig("MaxAuthTries"), e);
      }

      boolean auth = false;
      boolean auth_cancel = false;

      UserAuthNone uan = null;
      try {
        Class<? extends UserAuthNone> c =
            Class.forName(getConfig("userauth.none")).asSubclass(UserAuthNone.class);
        uan = c.getDeclaredConstructor().newInstance();
      } catch (Exception e) {
        throw new JSchException(e.toString(), e);
      }

      auth = uan.start(this);

      String cmethods = getConfig("PreferredAuthentications");

      String[] cmethoda = Util.split(cmethods, ",");

      String smethods = null;
      if (!auth) {
        smethods = uan.getMethods();
        if (smethods != null) {
          smethods = smethods.toLowerCase(Locale.ROOT);
        } else {
          // methods: publickey,password,keyboard-interactive
          // smethods = "publickey,password,keyboard-interactive";
          smethods = cmethods;
        }
      }

      String[] smethoda = Util.split(smethods, ",");

      int methodi = 0;

      loop: while (true) {

        while (!auth && cmethoda != null && methodi < cmethoda.length) {

          String method = cmethoda[methodi++];
          boolean acceptable = false;
          for (int k = 0; k < smethoda.length; k++) {
            if (smethoda[k].equals(method)) {
              acceptable = true;
              break;
            }
          }
          if (!acceptable) {
            continue;
          }

          // System.err.println(" method: " + method);

          if (getLogger().isEnabled(Logger.INFO)) {
            String str = "Authentications that can continue: ";
            for (int k = methodi - 1; k < cmethoda.length; k++) {
              str += cmethoda[k];
              if (k + 1 < cmethoda.length)
                str += ",";
            }
            getLogger().log(Logger.INFO, str);
            getLogger().log(Logger.INFO, "Next authentication method: " + method);
          }

          UserAuth ua = null;
          try {
            Class<? extends UserAuth> c = null;
            if (getConfig("userauth." + method) != null) {
              c = Class.forName(getConfig("userauth." + method)).asSubclass(UserAuth.class);
              ua = c.getDeclaredConstructor().newInstance();
            }
          } catch (Exception e) {
            if (getLogger().isEnabled(Logger.WARN)) {
              getLogger().log(Logger.WARN, "failed to load " + method + " method");
            }
          }

          if (ua != null) {
            auth_cancel = false;
            try {
              auth = ua.start(this);
              if (auth && getLogger().isEnabled(Logger.INFO)) {
                getLogger().log(Logger.INFO, "Authentication succeeded (" + method + ").");
              }
            } catch (JSchAuthCancelException ee) {
              auth_cancel = true;
            } catch (JSchPartialAuthException ee) {
              String tmp = smethods;
              smethods = ee.getMethods();
              smethoda = Util.split(smethods, ",");
              if (!tmp.equals(smethods)) {
                methodi = 0;
              }
              // System.err.println("PartialAuth: " + methods);
              auth_cancel = false;
              continue loop;
            } catch (RuntimeException ee) {
              throw ee;
            } catch (JSchException ee) {
              throw ee;
            } catch (Exception ee) {
              // SSH_MSG_DISCONNECT: Too many authentication failures
              // System.err.println("ee: " + ee);
              if (getLogger().isEnabled(Logger.WARN)) {
                getLogger().log(Logger.WARN, "an exception during authentication\n" + ee.toString(),
                    ee);
              }
              break loop;
            }
          }
        }
        break;
      }

      if (!auth) {
        if (auth_failures >= max_auth_tries) {
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO, "Login trials exceeds " + max_auth_tries);
          }
        }
        throw new JSchException(
            (auth_cancel ? "Auth cancel" : "Auth fail") + " for methods '" + smethods + "'");
      }

      if (socket != null && (connectTimeout > 0 || timeout > 0)) {
        socket.setSoTimeout(timeout);
      }

      isAuthed = true;

      synchronized (lock) {
        if (isConnected) {
          connectThread = getThreadFactory().newThread(this::run);
          connectThread.setName("Connect thread " + host + " session");
          if (daemon_thread) {
            connectThread.setDaemon(daemon_thread);
          }
          connectThread.start();

          requestPortForwarding();
        } else {
          // The session has been already down and
          // we don't have to start new thread.
        }
      }
    } catch (Exception e) {
      in_kex = false;
      try {
        if (isConnected) {
          String message = e.toString();
          packet.reset();
          buf.checkFreeSize(1 + 4 * 3 + message.length() + 2 + getBufferMargin());
          buf.putByte((byte) SSH_MSG_DISCONNECT);
          buf.putInt(3);
          buf.putString(Util.str2byte(message));
          buf.putString(Util.str2byte("en"));
          write(packet);
        }
      } catch (Exception ee) {
      }
      try {
        disconnect();
      } catch (Exception ee) {
      }
      isConnected = false;
      // e.printStackTrace();
      if (e instanceof RuntimeException)
        throw (RuntimeException) e;
      if (e instanceof JSchException)
        throw (JSchException) e;
      throw new JSchException("Session.connect: " + e, e);
    } finally {
      Util.bzero(this.password);
      this.password = null;
    }
  }

  private KeyExchange receive_kexinit(Buffer buf) throws Exception {
    int j = buf.getInt();
    if (j != buf.getLength()) { // packet was compressed and
      buf.getByte(); // j is the size of deflated packet.
      I_S = new byte[buf.index - 5];
    } else {
      I_S = new byte[j - 1 - buf.getByte()];
    }
    System.arraycopy(buf.buffer, buf.s, I_S, 0, I_S.length);

    if (initialKex) {
      if (enable_strict_kex || require_strict_kex) {
        doStrictKex = checkServerStrictKex();
        if (doStrictKex) {
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO, "Doing strict KEX");
          }

          if (seqi != 1) {
            throw new JSchStrictKexException("KEXINIT not first packet from server");
          }
        } else if (require_strict_kex) {
          throw new JSchStrictKexException("Strict KEX not supported by server");
        }
      }

      if (enable_server_sig_algs) {
        doExtInfo = checkServerExtInfo();
        if (doExtInfo && getLogger().isEnabled(Logger.INFO)) {
          getLogger().log(Logger.INFO, "ext-info messaging supported by server");
        }
      }
    }

    if (!in_kex) { // We are in rekeying activated by the remote!
      send_kexinit();
    }

    guess = KeyExchange.guess(this, I_S, I_C);

    if (guess[KeyExchange.PROPOSAL_KEX_ALGS].equals("ext-info-c")
        || guess[KeyExchange.PROPOSAL_KEX_ALGS].equals("ext-info-s")
        || guess[KeyExchange.PROPOSAL_KEX_ALGS].equals("kex-strict-c-v00@openssh.com")
        || guess[KeyExchange.PROPOSAL_KEX_ALGS].equals("kex-strict-s-v00@openssh.com")) {
      throw new JSchException("Invalid Kex negotiated: " + guess[KeyExchange.PROPOSAL_KEX_ALGS]);
    }

    if (!isAuthed && (guess[KeyExchange.PROPOSAL_ENC_ALGS_CTOS].equals("none")
        || (guess[KeyExchange.PROPOSAL_ENC_ALGS_STOC].equals("none")))) {
      throw new JSchException(
          "NONE Cipher should not be chosen before authentification is successed.");
    }

    KeyExchange kex = null;
    try {
      Class<? extends KeyExchange> c = Class
          .forName(getConfig(guess[KeyExchange.PROPOSAL_KEX_ALGS])).asSubclass(KeyExchange.class);
      kex = c.getDeclaredConstructor().newInstance();
    } catch (Exception | LinkageError e) {
      throw new JSchException(e.toString(), e);
    }

    kex.doInit(this, V_S, V_C, I_S, I_C);
    return kex;
  }

  boolean checkServerStrictKex() {
    Buffer sb = new Buffer(I_S);
    sb.setOffSet(17);
    byte[] sp = sb.getString(); // server proposal

    int l = 0;
    int m = 0;
    while (l < sp.length) {
      while (l < sp.length && sp[l] != ',')
        l++;
      if (m == l) {
        l++;
        m = l;
        continue;
      }
      if ("kex-strict-s-v00@openssh.com".equals(Util.byte2str(sp, m, l - m))) {
        return true;
      }
      l++;
      m = l;
    }
    return false;
  }

  boolean checkServerExtInfo() {
    Buffer sb = new Buffer(I_S);
    sb.setOffSet(17);
    byte[] sp = sb.getString(); // server proposal

    int l = 0;
    int m = 0;
    while (l < sp.length) {
      while (l < sp.length && sp[l] != ',')
        l++;
      if (m == l) {
        l++;
        m = l;
        continue;
      }
      if ("ext-info-s".equals(Util.byte2str(sp, m, l - m))) {
        return true;
      }
      l++;
      m = l;
    }

    return false;
  }

  private volatile boolean in_kex = false;
  private volatile boolean in_prompt = false;
  private volatile String[] not_available_shks = null;

  public String[] getUnavailableSignatures() {
    return not_available_shks;
  }

  public void rekey() throws Exception {
    send_kexinit();
  }

  private void send_kexinit() throws Exception {
    if (in_kex)
      return;

    String cipherc2s = getConfig("cipher.c2s");
    String ciphers2c = getConfig("cipher.s2c");
    String[] not_available_ciphers = checkCiphers(getConfig("CheckCiphers"));
    if (not_available_ciphers != null && not_available_ciphers.length > 0) {
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "cipher.c2s proposal before removing unavailable algos is: " + cipherc2s);
        getLogger().log(Logger.DEBUG,
            "cipher.s2c proposal before removing unavailable algos is: " + ciphers2c);
      }

      cipherc2s = Util.diffString(cipherc2s, not_available_ciphers);
      ciphers2c = Util.diffString(ciphers2c, not_available_ciphers);
      if (cipherc2s == null || ciphers2c == null) {
        throw new JSchException("There are not any available ciphers.");
      }

      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "cipher.c2s proposal after removing unavailable algos is: " + cipherc2s);
        getLogger().log(Logger.DEBUG,
            "cipher.s2c proposal after removing unavailable algos is: " + ciphers2c);
      }
    }

    String macc2s = getConfig("mac.c2s");
    String macs2c = getConfig("mac.s2c");
    String[] not_available_macs = checkMacs(getConfig("CheckMacs"));
    if (not_available_macs != null && not_available_macs.length > 0) {
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "mac.c2s proposal before removing unavailable algos is: " + macc2s);
        getLogger().log(Logger.DEBUG,
            "mac.s2c proposal before removing unavailable algos is: " + macs2c);
      }

      macc2s = Util.diffString(macc2s, not_available_macs);
      macs2c = Util.diffString(macs2c, not_available_macs);
      if (macc2s == null || macs2c == null) {
        throw new JSchException("There are not any available macs.");
      }

      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "mac.c2s proposal after removing unavailable algos is: " + macc2s);
        getLogger().log(Logger.DEBUG,
            "mac.s2c proposal after removing unavailable algos is: " + macs2c);
      }
    }

    String kex = getConfig("kex");
    String[] not_available_kexes = checkKexes(getConfig("CheckKexes"));
    if (not_available_kexes != null && not_available_kexes.length > 0) {
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG, "kex proposal before removing unavailable algos is: " + kex);
      }

      kex = Util.diffString(kex, not_available_kexes);
      if (kex == null) {
        throw new JSchException("There are not any available kexes.");
      }

      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG, "kex proposal after removing unavailable algos is: " + kex);
      }
    }

    if (enable_server_sig_algs && !isAuthed) {
      kex += ",ext-info-c";
    }

    if ((enable_strict_kex || require_strict_kex) && initialKex) {
      kex += ",kex-strict-c-v00@openssh.com";
    }

    String server_host_key = getConfig("server_host_key");
    String[] not_available_shks = checkSignatures(getConfig("CheckSignatures"));
    // Cache for UserAuthPublicKey
    this.not_available_shks = not_available_shks;
    if (not_available_shks != null && not_available_shks.length > 0) {
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "server_host_key proposal before removing unavailable algos is: " + server_host_key);
      }

      server_host_key = Util.diffString(server_host_key, not_available_shks);
      if (server_host_key == null) {
        throw new JSchException("There are not any available sig algorithm.");
      }

      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "server_host_key proposal after removing unavailable algos is: " + server_host_key);
      }

      // Also filter out certificate types for unavailable base algorithms
      server_host_key =
          OpenSshCertificateUtil.filterUnavailableCertTypes(server_host_key, not_available_shks);
      if (server_host_key == null) {
        throw new JSchException("There are not any available signature algorithms.");
      }

      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "server_host_key proposal after removing unavailable cert algos is: "
                + server_host_key);
      }
    }

    String prefer_hkr = getConfig("prefer_known_host_key_types");
    if (prefer_hkr.equals("yes")) {
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "server_host_key proposal before known_host reordering is: " + server_host_key);
      }

      HostKeyRepository hkr = getHostKeyRepository();
      String chost = host;
      if (hostKeyAlias != null) {
        chost = hostKeyAlias;
      }
      if (hostKeyAlias == null && port != 22) {
        chost = ("[" + chost + "]:" + port);
      }
      HostKey[] hks = hkr.getHostKey(chost, null);
      if (hks != null && hks.length > 0) {
        List<String> pref_shks = new ArrayList<>();
        List<String> shks = new ArrayList<>(Arrays.asList(Util.split(server_host_key, ",")));
        Iterator<String> it = shks.iterator();
        while (it.hasNext()) {
          String algo = it.next();
          String type = algo;

          // Normalize certificate types to base types for matching against known_hosts
          // This aligns with OpenSSH behavior where certificate algorithms are matched
          // against their base key types (e.g., ssh-rsa-cert-v01@openssh.com -> ssh-rsa)
          // See:
          // https://github.com/openssh/openssh-portable/blob/5f98660c51e673f521e0216c7ed20205c4af10ed/sshconnect2.c#L186-L189
          String baseType = OpenSshCertificateKeyTypes.getBaseKeyType(type);
          if (baseType != null && !baseType.equals(type)) {
            type = baseType;
          }

          // Normalize RSA signature variants to base type
          if (type.equals("rsa-sha2-256") || type.equals("rsa-sha2-512")
              || type.equals("ssh-rsa-sha224@ssh.com") || type.equals("ssh-rsa-sha256@ssh.com")
              || type.equals("ssh-rsa-sha384@ssh.com") || type.equals("ssh-rsa-sha512@ssh.com")) {
            type = "ssh-rsa";
          }

          for (HostKey hk : hks) {
            if (hk.getType().equals(type)) {
              pref_shks.add(algo);
              it.remove();
              break;
            }
          }
        }
        if (pref_shks.size() > 0) {
          pref_shks.addAll(shks);
          server_host_key = String.join(",", pref_shks);
        }
      }

      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "server_host_key proposal after known_host reordering is: " + server_host_key);
      }
    }

    kex_start_time = System.currentTimeMillis();
    in_kex = true;

    // byte SSH_MSG_KEXINIT(20)
    // byte[16] cookie (random bytes)
    // string kex_algorithms
    // string server_host_key_algorithms
    // string encryption_algorithms_client_to_server
    // string encryption_algorithms_server_to_client
    // string mac_algorithms_client_to_server
    // string mac_algorithms_server_to_client
    // string compression_algorithms_client_to_server
    // string compression_algorithms_server_to_client
    // string languages_client_to_server
    // string languages_server_to_client
    Buffer buf = new Buffer(); // send_kexinit may be invoked
    Packet packet = new Packet(buf); // by user thread.
    packet.reset();
    buf.putByte((byte) SSH_MSG_KEXINIT);
    synchronized (random) {
      random.fill(buf.buffer, buf.index, 16);
      buf.skip(16);
    }
    buf.putString(Util.str2byte(kex));
    buf.putString(Util.str2byte(server_host_key));
    buf.putString(Util.str2byte(cipherc2s));
    buf.putString(Util.str2byte(ciphers2c));
    buf.putString(Util.str2byte(getConfig("mac.c2s")));
    buf.putString(Util.str2byte(getConfig("mac.s2c")));
    buf.putString(Util.str2byte(getConfig("compression.c2s")));
    buf.putString(Util.str2byte(getConfig("compression.s2c")));
    buf.putString(Util.str2byte(getConfig("lang.c2s")));
    buf.putString(Util.str2byte(getConfig("lang.s2c")));
    buf.putByte((byte) 0);
    buf.putInt(0);

    buf.setOffSet(5);
    I_C = new byte[buf.getLength()];
    buf.getByte(I_C);

    write(packet);

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "SSH_MSG_KEXINIT sent");
    }
  }

  private void send_newkeys() throws Exception {
    // send SSH_MSG_NEWKEYS(21)
    packet.reset();
    buf.putByte((byte) SSH_MSG_NEWKEYS);
    write(packet);

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "SSH_MSG_NEWKEYS sent");
    }
  }

  private void send_extinfo() throws Exception {
    // send SSH_MSG_EXT_INFO(7)
    packet.reset();
    buf.putByte((byte) SSH_MSG_EXT_INFO);
    buf.putInt(1);
    buf.putString(Util.str2byte("ext-info-in-auth@openssh.com"));
    buf.putString(Util.str2byte("0"));
    write(packet);

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "SSH_MSG_EXT_INFO sent");
    }
  }

  private void checkHost(String chost, int port, KeyExchange kex) throws JSchException {
    if (hostKeyAlias != null) {
      chost = hostKeyAlias;
    } else if (port != 22) {
      chost = "[" + chost + "]:" + port;
    }

    OpenSshCertificate certificate = kex.getHostKeyCertificate();
    if (certificate != null) {
      try {
        OpenSshCertificateHostKeyVerifier.checkHostCertificate(this, certificate);
        if (getLogger().isEnabled(Logger.INFO)) {
          getLogger().log(Logger.INFO, "Host '" + chost + "' is known and matches the "
              + kex.getKeyType() + " host certificate");
        }
        return;
      } catch (JSchRevokedHostKeyException e) {
        // Revoked key is a hard failure — never fall back to plain key checking
        // This matches OpenSSH behavior where @revoked is a terminal rejection
        if (userinfo != null) {
          userinfo.showMessage("WARNING: REVOKED HOST KEY DETECTED!\n"
              + "A key associated with the host certificate for " + chost
              + " is marked as revoked.\n"
              + "This could mean that a stolen key is being used to impersonate this host.");
        }
        if (getLogger().isEnabled(Logger.INFO)) {
          getLogger().log(Logger.INFO,
              "Host '" + chost + "' has provided a certificate with a revoked key.");
        }
        throw e;
      } catch (JSchException e) {
        if (getConfig("host_certificate_to_key_fallback").equals("no")) {
          throw e;
        }
        // Fallback to public key verification - log warning as this bypasses CA validation
        if (getLogger().isEnabled(Logger.INFO)) {
          getLogger().log(Logger.INFO,
              "Host certificate validation failed, falling back to public key verification. "
                  + "This bypasses CA trust validation. Reason: " + e.getMessage(),
              e);
        }
        byte[] K_S = certificate.getCertificatePublicKey();
        if (K_S == null) {
          throw new JSchException(
              "Invalid certificate '" + certificate.getId() + "': missing public key");
        }
        String key_type = kex.getKeyType();
        String key_footprint = kex.getFingerPrint();
        String keyAlgorithmName = kex.getKeyAlgorithName();
        doCheckHostKey(chost, key_type, key_footprint, keyAlgorithmName, K_S);
        return;
      }
    }

    byte[] K_S = kex.getHostKey();
    String key_type = kex.getKeyType();
    String key_fprint = kex.getFingerPrint();
    String keyAlgorithmName = kex.getKeyAlgorithName();

    doCheckHostKey(chost, key_type, key_fprint, keyAlgorithmName, K_S);
  }

  private void doCheckHostKey(String chost, String key_type, String key_fprint,
      String keyAlgorithmName, byte[] K_S) throws JSchException {

    String shkc = getConfig("StrictHostKeyChecking");
    // System.err.println("shkc: "+shkc);

    HostKeyRepository hkr = getHostKeyRepository();

    String hkh = getConfig("HashKnownHosts");
    if (hkh.equals("yes") && (hkr instanceof KnownHosts)) {
      hostkey = ((KnownHosts) hkr).createHashedHostKey(chost, K_S);
    } else {
      hostkey = new HostKey(chost, K_S);
    }

    int i = 0;
    synchronized (hkr) {
      i = hkr.check(chost, K_S);
    }

    boolean insert = false;
    if ((shkc.equals("ask") || shkc.equals("yes")) && i == HostKeyRepository.CHANGED) {
      String file = null;
      synchronized (hkr) {
        file = hkr.getKnownHostsRepositoryID();
      }
      if (file == null) {
        file = "known_hosts";
      }

      boolean b = false;

      if (userinfo != null) {
        String message = "WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!\n"
            + "IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!\n"
            + "Someone could be eavesdropping on you right now (man-in-the-middle attack)!\n"
            + "It is also possible that the " + key_type + " host key has just been changed.\n"
            + "The fingerprint for the " + key_type + " key sent by the remote host " + chost
            + " is\n" + key_fprint + ".\n" + "Please contact your system administrator.\n"
            + "Add correct host key in " + file + " to get rid of this message.";

        if (shkc.equals("ask")) {
          b = userinfo
              .promptYesNo(message + "\nDo you want to delete the old key and insert the new key?");
        } else { // shkc.equals("yes")
          userinfo.showMessage(message);
        }
      }

      if (!b) {
        throw new JSchChangedHostKeyException("HostKey has been changed: " + chost);
      }

      synchronized (hkr) {
        hkr.remove(chost, keyAlgorithmName, null);
        insert = true;
      }
    }

    if ((shkc.equals("ask") || shkc.equals("yes")) && (i != HostKeyRepository.OK) && !insert) {
      if (shkc.equals("yes")) {
        throw new JSchUnknownHostKeyException("reject HostKey: " + chost);
      }
      // System.err.println("finger-print: "+key_fprint);
      if (userinfo != null) {
        boolean foo = userinfo.promptYesNo("The authenticity of host '" + chost
            + "' can't be established.\n" + key_type + " key fingerprint is " + key_fprint + ".\n"
            + "Are you sure you want to continue connecting?");
        if (!foo) {
          throw new JSchUnknownHostKeyException("reject HostKey: " + chost);
        }
        insert = true;
      } else {
        if (i == HostKeyRepository.NOT_INCLUDED)
          throw new JSchUnknownHostKeyException(
              "UnknownHostKey: " + chost + ". " + key_type + " key fingerprint is " + key_fprint);
        else
          throw new JSchChangedHostKeyException("HostKey has been changed: " + chost);
      }
    }

    if (shkc.equals("no") && HostKeyRepository.NOT_INCLUDED == i) {
      insert = true;
    }

    if (i == HostKeyRepository.OK) {
      HostKey[] keys = hkr.getHostKey(chost, keyAlgorithmName);
      String _key = Util.byte2str(Util.toBase64(K_S, 0, K_S.length, true));
      for (int j = 0; j < keys.length; j++) {
        if (keys[j].getKey().equals(_key) && keys[j].getMarker().equals("@revoked")) {
          if (userinfo != null) {
            userinfo.showMessage("The " + key_type + " host key for " + chost
                + " is marked as revoked.\n" + "This could mean that a stolen key is being used to "
                + "impersonate this host.");
          }
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO, "Host '" + chost + "' has provided revoked key.");
          }
          throw new JSchRevokedHostKeyException("revoked HostKey: " + chost);
        }
      }
    }

    if (i == HostKeyRepository.OK && getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO,
          "Host '" + chost + "' is known and matches the " + key_type + " host key");
    }

    if (insert && getLogger().isEnabled(Logger.WARN)) {
      getLogger().log(Logger.WARN,
          "Permanently added '" + chost + "' (" + key_type + ") to the list of known hosts.");
    }

    if (insert) {
      synchronized (hkr) {
        hkr.add(hostkey, userinfo);
      }
    }
  }

  // public void start(){ (new Thread(this)).start(); }

  public Channel openChannel(String type) throws JSchException {
    if (!isConnected) {
      throw new JSchException("session is down");
    }
    try {
      Channel channel = createChannel(type);
      if (channel != null) {
        channel.init();
        if (channel instanceof ChannelSession) {
          applyConfigChannel((ChannelSession) channel);
        }
        return channel;
      } else {
        if (getLogger().isEnabled(Logger.DEBUG)) {
          getLogger().log(Logger.DEBUG, "Failed to open channel of type " + type
              + " - type unsupported or session may be disconnecting");
        }
      }
    } catch (Exception e) {
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG,
            "Exception creating channel of type " + type + ": " + e.getMessage(), e);
      }
    }
    return null;
  }

  // encode will bin invoked in write with synchronization.
  void encode(Packet packet) throws Exception {
    // System.err.println("encode: "+packet.buffer.getCommand());
    // System.err.println(" "+packet.buffer.index);
    // if(packet.buffer.getCommand()==96){
    // Thread.dumpStack();
    // }
    if (deflater != null) {
      compress_len[0] = packet.buffer.index;
      packet.buffer.buffer = deflater.compress(packet.buffer.buffer, 5, compress_len);
      packet.buffer.index = compress_len[0];
    }
    int bsize = 8;
    if (c2scipher != null) {
      // bsize=c2scipher.getIVSize();
      bsize = c2scipher_size;
    }
    boolean isChaCha20 = (c2scipher != null && c2scipher.isChaCha20());
    boolean isAEAD = (c2scipher != null && c2scipher.isAEAD());
    boolean isEtM =
        (!isChaCha20 && !isAEAD && c2scipher != null && c2smac != null && c2smac.isEtM());
    packet.padding(bsize, !(isChaCha20 || isAEAD || isEtM));

    byte[] buf = packet.buffer.buffer;
    if (isChaCha20) {
      // init cipher with seq number
      c2scipher.update(seqo);
      // encrypt packet length field
      c2scipher.update(buf, 0, 4, buf, 0);
      // encrypt rest of packet & add tag
      c2scipher.doFinal(buf, 0, packet.buffer.index, buf, 0);
      packet.buffer.skip(c2scipher.getTagSize());
    } else if (isAEAD) {
      c2scipher.updateAAD(buf, 0, 4);
      c2scipher.doFinal(buf, 4, packet.buffer.index - 4, buf, 4);
      packet.buffer.skip(c2scipher.getTagSize());
    } else if (isEtM) {
      c2scipher.update(buf, 4, packet.buffer.index - 4, buf, 4);
      c2smac.update(seqo);
      c2smac.update(packet.buffer.buffer, 0, packet.buffer.index);
      c2smac.doFinal(packet.buffer.buffer, packet.buffer.index);
      packet.buffer.skip(c2smac.getBlockSize());
    } else {
      if (c2smac != null) {
        c2smac.update(seqo);
        c2smac.update(packet.buffer.buffer, 0, packet.buffer.index);
        c2smac.doFinal(packet.buffer.buffer, packet.buffer.index);
      }
      if (c2scipher != null) {
        c2scipher.update(buf, 0, packet.buffer.index, buf, 0);
      }
      if (c2smac != null) {
        packet.buffer.skip(c2smac.getBlockSize());
      }
    }
  }

  int[] uncompress_len = new int[1];
  int[] compress_len = new int[1];

  private int s2ccipher_size = 8;
  private int c2scipher_size = 8;

  private Channel getChannelById(int id) {
    Lock l = channelsLock.readLock();
    l.lock();
    try {
      for (Channel c : channels) {
        if (c.id == id) {
          return c;
        }
      }
    } finally {
      l.unlock();
    }
    return null;
  }

  Buffer read(Buffer buf) throws Exception {
    // determine which encryption and authentication mode is currently active
    // for the server-to-client (s2c) connection
    int j = 0;
    boolean isChaCha20 = (s2ccipher != null && s2ccipher.isChaCha20());
    boolean isAEAD = (s2ccipher != null && s2ccipher.isAEAD());
    boolean isEtM =
        (!isChaCha20 && !isAEAD && s2ccipher != null && s2cmac != null && s2cmac.isEtM());

    // Decrypting
    while (true) {
      buf.reset();
      if (isChaCha20) {
        // read encrypted packet length field
        io.getByte(buf.buffer, buf.index, 4);
        buf.index += 4;
        // init cipher with seq number
        s2ccipher.update(seqi);
        // decrypt packet length field
        byte[] tmp = new byte[4];
        s2ccipher.update(buf.buffer, 0, 4, tmp, 0);
        j = ((tmp[0] << 24) & 0xff000000) | ((tmp[1] << 16) & 0x00ff0000)
            | ((tmp[2] << 8) & 0x0000ff00) | ((tmp[3]) & 0x000000ff);
        // RFC 4253 6.1. Maximum Packet Length
        if (j < 5 || j > PACKET_MAX_SIZE) {
          start_discard(buf, s2ccipher, s2cmac, 0, PACKET_MAX_SIZE);
        }
        j += s2ccipher.getTagSize();
        if ((buf.index + j) > buf.buffer.length) {
          byte[] foo = new byte[buf.index + j];
          System.arraycopy(buf.buffer, 0, foo, 0, buf.index);
          buf.buffer = foo;
        }

        if ((j % s2ccipher_size) != 0) {
          String message = "Bad packet length " + j;
          if (getLogger().isEnabled(Logger.FATAL)) {
            getLogger().log(Logger.FATAL, message);
          }
          start_discard(buf, s2ccipher, s2cmac, 0, PACKET_MAX_SIZE - s2ccipher_size);
        }

        io.getByte(buf.buffer, buf.index, j);
        // subtract tag size now that whole packet has been fetched
        j -= s2ccipher.getTagSize();
        buf.index += (j);
        try {
          s2ccipher.doFinal(buf.buffer, 0, j + 4, buf.buffer, 0);
        } catch (AEADBadTagException e) {
          throw new JSchException("Packet corrupt", e);
        }
        // overwrite encrypted packet length field with decrypted version
        System.arraycopy(tmp, 0, buf.buffer, 0, 4);
      } else if (isAEAD || isEtM) {
        io.getByte(buf.buffer, buf.index, 4);
        buf.index += 4;
        j = ((buf.buffer[0] << 24) & 0xff000000) | ((buf.buffer[1] << 16) & 0x00ff0000)
            | ((buf.buffer[2] << 8) & 0x0000ff00) | ((buf.buffer[3]) & 0x000000ff);
        // RFC 4253 6.1. Maximum Packet Length
        if (j < 5 || j > PACKET_MAX_SIZE) {
          start_discard(buf, s2ccipher, s2cmac, 0, PACKET_MAX_SIZE);
        }
        if (isAEAD) {
          j += s2ccipher.getTagSize();
        }
        if ((buf.index + j) > buf.buffer.length) {
          byte[] foo = new byte[buf.index + j];
          System.arraycopy(buf.buffer, 0, foo, 0, buf.index);
          buf.buffer = foo;
        }

        if ((j % s2ccipher_size) != 0) {
          String message = "Bad packet length " + j;
          if (getLogger().isEnabled(Logger.FATAL)) {
            getLogger().log(Logger.FATAL, message);
          }
          start_discard(buf, s2ccipher, s2cmac, 0, PACKET_MAX_SIZE - s2ccipher_size);
        }

        io.getByte(buf.buffer, buf.index, j);
        buf.index += (j);

        if (isAEAD) {
          try {
            s2ccipher.updateAAD(buf.buffer, 0, 4);
            s2ccipher.doFinal(buf.buffer, 4, j, buf.buffer, 4);
          } catch (AEADBadTagException e) {
            throw new JSchException("Packet corrupt", e);
          }
          // don't include AEAD tag size in buf so that decompression works below
          buf.index -= s2ccipher.getTagSize();
        } else {
          s2cmac.update(seqi);
          s2cmac.update(buf.buffer, 0, buf.index);
          s2cmac.doFinal(s2cmac_result1, 0);

          io.getByte(s2cmac_result2, 0, s2cmac_result2.length);
          if (!Util.arraysequals(s2cmac_result1, s2cmac_result2)) {
            throw new JSchException("Packet corrupt");
          }
          s2ccipher.update(buf.buffer, 4, j, buf.buffer, 4);
        }
      } else {
        // fall back to the older Encrypt-and-MAC mode.
        io.getByte(buf.buffer, buf.index, s2ccipher_size);
        buf.index += s2ccipher_size;
        if (s2ccipher != null) {
          s2ccipher.update(buf.buffer, 0, s2ccipher_size, buf.buffer, 0);
        }

        // calculating length of the incoming packet
        j = ((buf.buffer[0] << 24) & 0xff000000) | ((buf.buffer[1] << 16) & 0x00ff0000)
            | ((buf.buffer[2] << 8) & 0x0000ff00) | ((buf.buffer[3]) & 0x000000ff);
        // RFC 4253 6.1. Maximum Packet Length
        if (j < 5 || j > PACKET_MAX_SIZE) {
          start_discard(buf, s2ccipher, s2cmac, 0, PACKET_MAX_SIZE);
        }

        // calculates how many more bytes need to be read from the buffer to get the rest of the
        // encrypted SSH packet
        int need = j + 4 - s2ccipher_size;
        // if(need<0){
        // throw new IOException("invalid data");
        // }
        if ((buf.index + need) > buf.buffer.length) {
          byte[] foo = new byte[buf.index + need];
          System.arraycopy(buf.buffer, 0, foo, 0, buf.index);
          buf.buffer = foo;
        }

        if ((need % s2ccipher_size) != 0) {
          String message = "Bad packet length " + need;
          if (getLogger().isEnabled(Logger.FATAL)) {
            getLogger().log(Logger.FATAL, message);
          }
          start_discard(buf, s2ccipher, s2cmac, 0, PACKET_MAX_SIZE - s2ccipher_size);
        }

        if (need > 0) {
          io.getByte(buf.buffer, buf.index, need);
          buf.index += (need);
          if (s2ccipher != null) {
            s2ccipher.update(buf.buffer, s2ccipher_size, need, buf.buffer, s2ccipher_size);
          }
        }

        if (s2cmac != null) {
          s2cmac.update(seqi);
          s2cmac.update(buf.buffer, 0, buf.index);
          s2cmac.doFinal(s2cmac_result1, 0);

          io.getByte(s2cmac_result2, 0, s2cmac_result2.length);
          if (!Util.arraysequals(s2cmac_result1, s2cmac_result2)) {
            if (need + s2ccipher_size > PACKET_MAX_SIZE) {
              throw new IOException("MAC Error");
            }
            start_discard(buf, s2ccipher, s2cmac, buf.index,
                PACKET_MAX_SIZE - need - s2ccipher_size);
            continue;
          }
        }
      }

      if (++seqi == 0 && (enable_strict_kex || require_strict_kex) && initialKex) {
        throw new JSchStrictKexException("incoming sequence number wrapped during initial KEX");
      }

      if (inflater != null) {
        try {
          // inflater.uncompress(buf);
          int pad = buf.buffer[4];
          uncompress_len[0] = buf.index - 5 - pad;
          byte[] foo = inflater.uncompress(buf.buffer, 5, uncompress_len);
          if (foo != null) {
            buf.buffer = foo;
            buf.index = 5 + uncompress_len[0];
          } else {
            if (getLogger().isEnabled(Logger.ERROR)) {
              getLogger().log(Logger.ERROR, "fail in inflater");
            }
            break;
          }
        } catch (Compression.InflaterException e) {
          throw new JSchException(e.getMessage(), e);
        }
      }

      int type = buf.getCommand() & 0xff;
      // System.err.println("read: "+type);
      if (type == SSH_MSG_DISCONNECT) {
        buf.rewind();
        buf.getInt();
        buf.getShort();
        int reason_code = buf.getInt();
        byte[] description_array = buf.getString();
        byte[] language_tag_array = buf.getString();
        String description = Util.byte2str(description_array);
        String language_tag = Util.byte2str(language_tag_array);
        throw new JSchSessionDisconnectException(
            "SSH_MSG_DISCONNECT: " + reason_code + " " + description + " " + language_tag,
            reason_code, description, language_tag);
        // break;
      } else if (initialKex && doStrictKex) {
        break;
      } else if (type == SSH_MSG_IGNORE) {
      } else if (type == SSH_MSG_UNIMPLEMENTED) {
        buf.rewind();
        buf.getInt();
        buf.getShort();
        int reason_id = buf.getInt();
        if (getLogger().isEnabled(Logger.INFO)) {
          getLogger().log(Logger.INFO, "Received SSH_MSG_UNIMPLEMENTED for " + reason_id);
        }
      } else if (type == SSH_MSG_DEBUG) {
        buf.rewind();
        buf.getInt();
        buf.getShort();
        /*
         * byte always_display=(byte)buf.getByte(); byte[] message=buf.getString(); byte[]
         * language_tag=buf.getString(); System.err.println("SSH_MSG_DEBUG:"+
         * " "+Util.byte2str(message)+ " "+Util.byte2str(language_tag));
         */
      } else if (type == SSH_MSG_CHANNEL_WINDOW_ADJUST) {
        buf.rewind();
        buf.getInt();
        buf.getShort();
        Channel c = getChannelById(buf.getInt());
        if (c != null) {
          c.addRemoteWindowSize(buf.getUInt());
        }
      } else if (type == SSH_MSG_EXT_INFO) {
        buf.rewind();
        buf.getInt();
        buf.getShort();
        boolean ignore = false;
        if (!enable_server_sig_algs) {
          ignore = true;
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO,
                "Ignoring SSH_MSG_EXT_INFO while enable_server_sig_algs != yes");
          }
        } else if (isAuthed) {
          ignore = true;
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO,
                "Ignoring SSH_MSG_EXT_INFO received after SSH_MSG_USERAUTH_SUCCESS");
          }
        } else if (in_kex) {
          ignore = true;
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO,
                "Ignoring SSH_MSG_EXT_INFO received before SSH_MSG_NEWKEYS");
          }
        } else {
          if (getLogger().isEnabled(Logger.INFO)) {
            getLogger().log(Logger.INFO, "SSH_MSG_EXT_INFO received");
          }
        }
        long num_extensions = buf.getUInt();
        for (long i = 0; i < num_extensions; i++) {
          byte[] ext_name = buf.getString();
          byte[] ext_value = buf.getString();
          if (!ignore && Util.byte2str(ext_name).equals("server-sig-algs")) {
            String foo = Util.byte2str(ext_value);
            if (getLogger().isEnabled(Logger.INFO)) {
              getLogger().log(Logger.INFO, "server-sig-algs=<" + foo + ">");
            }
            if (sshBugSigType74) {
              if (!foo.isEmpty()) {
                foo += ",rsa-sha2-256,rsa-sha2-512";
              } else {
                foo = "rsa-sha2-256,rsa-sha2-512";
              }
              if (getLogger().isEnabled(Logger.INFO)) {
                getLogger().log(Logger.INFO,
                    "OpenSSH 7.4 detected: adding rsa-sha2-256 & rsa-sha2-512 to server-sig-algs");
              }
            }
            serverSigAlgs = Util.split(foo, ",");
          }
        }
      } else if (type == UserAuth.SSH_MSG_USERAUTH_SUCCESS) {
        isAuthed = true;
        if (inflater == null && deflater == null) {
          String method;
          method = guess[KeyExchange.PROPOSAL_COMP_ALGS_CTOS];
          initDeflater(method);
          method = guess[KeyExchange.PROPOSAL_COMP_ALGS_STOC];
          initInflater(method);
        }
        break;
      } else {
        break;
      }
    }
    buf.rewind();
    return buf;
  }

  private void start_discard(Buffer buf, Cipher cipher, MAC mac, int mac_already, int discard)
      throws JSchException {
    if (!cipher.isCBC() || (mac != null && mac.isEtM())) {
      throw new JSchException("Packet corrupt");
    }

    if (mac != null) {
      mac.update(seqi);
      mac.update(buf.buffer, 0, mac_already);
    }

    IOException ioe = null;
    try {
      while (discard > 0) {
        buf.reset();
        int len = discard > buf.buffer.length ? buf.buffer.length : discard;
        io.getByte(buf.buffer, 0, len);
        if (mac != null) {
          mac.update(buf.buffer, 0, len);
        }
        discard -= len;
      }
    } catch (IOException e) {
      ioe = e;
      if (getLogger().isEnabled(Logger.ERROR)) {
        getLogger().log(Logger.ERROR, "start_discard finished early due to " + e.getMessage(), e);
      }
    }

    if (mac != null) {
      mac.doFinal(buf.buffer, 0);
    }

    JSchException e = new JSchException("Packet corrupt");
    if (ioe != null) {
      e.addSuppressed(ioe);
    }
    throw e;
  }

  byte[] getSessionId() {
    return session_id;
  }

  private void receive_newkeys(Buffer buf, KeyExchange kex) throws Exception {
    try {
      updateKeys(kex);
    } finally {
      kex.clearK();
    }
    in_kex = false;
    if (doStrictKex) {
      seqi = 0;
      if (getLogger().isEnabled(Logger.INFO)) {
        getLogger().log(Logger.INFO,
            "Reset incoming sequence number after receiving SSH_MSG_NEWKEYS for strict KEX");
      }
    }
  }

  private void updateKeys(KeyExchange kex) throws Exception {
    byte[] K = kex.getK();
    byte[] H = kex.getH();
    HASH hash = kex.getHash();

    if (session_id == null) {
      session_id = new byte[H.length];
      System.arraycopy(H, 0, session_id, 0, H.length);
    }

    /*
     * Initial IV client to server: HASH (K || H || "A" || session_id) Initial IV server to client:
     * HASH (K || H || "B" || session_id) Encryption key client to server: HASH (K || H || "C" ||
     * session_id) Encryption key server to client: HASH (K || H || "D" || session_id) Integrity key
     * client to server: HASH (K || H || "E" || session_id) Integrity key server to client: HASH (K
     * || H || "F" || session_id)
     */

    buf.reset();
    buf.putByte(K);
    buf.putByte(H);
    buf.putByte((byte) 0x41);
    buf.putByte(session_id);
    hash.update(buf.buffer, 0, buf.index);
    IVc2s = hash.digest();

    int j = buf.index - session_id.length - 1;

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    IVs2c = hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    Ec2s = hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    Es2c = hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    MACc2s = hash.digest();

    buf.buffer[j]++;
    hash.update(buf.buffer, 0, buf.index);
    MACs2c = hash.digest();

    try {
      Class<? extends Cipher> cc;
      Class<? extends MAC> cm;
      String method;

      method = guess[KeyExchange.PROPOSAL_ENC_ALGS_STOC];
      cc = Class.forName(getConfig(method)).asSubclass(Cipher.class);
      s2ccipher = cc.getDeclaredConstructor().newInstance();
      while (s2ccipher.getBlockSize() > Es2c.length) {
        buf.reset();
        buf.putByte(K);
        buf.putByte(H);
        buf.putByte(Es2c);
        hash.update(buf.buffer, 0, buf.index);
        byte[] foo = hash.digest();
        byte[] bar = new byte[Es2c.length + foo.length];
        System.arraycopy(Es2c, 0, bar, 0, Es2c.length);
        System.arraycopy(foo, 0, bar, Es2c.length, foo.length);
        Es2c = bar;
      }
      s2ccipher.init(Cipher.DECRYPT_MODE, Es2c, IVs2c);
      s2ccipher_size = s2ccipher.getIVSize();

      if (!s2ccipher.isAEAD()) {
        method = guess[KeyExchange.PROPOSAL_MAC_ALGS_STOC];
        cm = Class.forName(getConfig(method)).asSubclass(MAC.class);
        s2cmac = cm.getDeclaredConstructor().newInstance();
        MACs2c = expandKey(buf, K, H, MACs2c, hash, s2cmac.getBlockSize());
        s2cmac.init(MACs2c);
        // mac_buf=new byte[s2cmac.getBlockSize()];
        s2cmac_result1 = new byte[s2cmac.getBlockSize()];
        s2cmac_result2 = new byte[s2cmac.getBlockSize()];
      }

      method = guess[KeyExchange.PROPOSAL_ENC_ALGS_CTOS];
      cc = Class.forName(getConfig(method)).asSubclass(Cipher.class);
      c2scipher = cc.getDeclaredConstructor().newInstance();
      while (c2scipher.getBlockSize() > Ec2s.length) {
        buf.reset();
        buf.putByte(K);
        buf.putByte(H);
        buf.putByte(Ec2s);
        hash.update(buf.buffer, 0, buf.index);
        byte[] foo = hash.digest();
        byte[] bar = new byte[Ec2s.length + foo.length];
        System.arraycopy(Ec2s, 0, bar, 0, Ec2s.length);
        System.arraycopy(foo, 0, bar, Ec2s.length, foo.length);
        Ec2s = bar;
      }
      c2scipher.init(Cipher.ENCRYPT_MODE, Ec2s, IVc2s);
      c2scipher_size = c2scipher.getIVSize();

      if (!c2scipher.isAEAD()) {
        method = guess[KeyExchange.PROPOSAL_MAC_ALGS_CTOS];
        cm = Class.forName(getConfig(method)).asSubclass(MAC.class);
        c2smac = cm.getDeclaredConstructor().newInstance();
        MACc2s = expandKey(buf, K, H, MACc2s, hash, c2smac.getBlockSize());
        c2smac.init(MACc2s);
      }

      method = guess[KeyExchange.PROPOSAL_COMP_ALGS_CTOS];
      initDeflater(method);

      method = guess[KeyExchange.PROPOSAL_COMP_ALGS_STOC];
      initInflater(method);
    } catch (Exception | LinkageError e) {
      if (e instanceof JSchException)
        throw e;
      throw new JSchException(e.toString(), e);
      // System.err.println("updatekeys: "+e);
    }
  }

  /*
   * RFC 4253 7.2. Output from Key Exchange If the key length needed is longer than the output of
   * the HASH, the key is extended by computing HASH of the concatenation of K and H and the entire
   * key so far, and appending the resulting bytes (as many as HASH generates) to the key. This
   * process is repeated until enough key material is available; the key is taken from the beginning
   * of this value. In other words: K1 = HASH(K || H || X || session_id) (X is e.g., "A") K2 =
   * HASH(K || H || K1) K3 = HASH(K || H || K1 || K2) ... key = K1 || K2 || K3 || ...
   */
  private byte[] expandKey(Buffer buf, byte[] K, byte[] H, byte[] key, HASH hash,
      int required_length) throws Exception {
    byte[] result = key;
    int size = hash.getBlockSize();
    while (result.length < required_length) {
      buf.reset();
      buf.putByte(K);
      buf.putByte(H);
      buf.putByte(result);
      hash.update(buf.buffer, 0, buf.index);
      byte[] tmp = new byte[result.length + size];
      System.arraycopy(result, 0, tmp, 0, result.length);
      System.arraycopy(hash.digest(), 0, tmp, result.length, size);
      Util.bzero(result);
      result = tmp;
    }
    return result;
  }

  /* synchronized */ void write(Packet packet, Channel c, int length) throws Exception {
    long t = getTimeout();
    while (true) {
      if (in_kex) {
        if (t > 0L && (System.currentTimeMillis() - kex_start_time) > t) {
          throw new JSchException("timeout in waiting for rekeying process.");
        }
        try {
          Thread.sleep(10);
        } catch (InterruptedException e) {
        } ;
        continue;
      }
      synchronized (c) {
        if (c.rwsize < length) {
          try {
            c.notifyme++;
            c.wait(100);
          } catch (InterruptedException e) {
          } finally {
            c.notifyme--;
          }
        }

        if (in_kex) {
          continue;
        }

        if (c.rwsize >= length) {
          c.rwsize -= length;
          break;
        }
      }
      if (c.close || !c.isConnected()) {
        throw new IOException("channel is broken");
      }

      boolean sendit = false;
      int s = 0;
      byte command = 0;
      int recipient = -1;
      synchronized (c) {
        if (c.rwsize > 0) {
          long len = c.rwsize;
          if (len > length) {
            len = length;
          }
          if (len != length) {
            s = packet.shift((int) len, (c2scipher != null ? c2scipher_size : 8),
                (c2smac != null ? c2smac.getBlockSize() : 0));
          }
          command = packet.buffer.getCommand();
          recipient = c.getRecipient();
          length -= (int) len;
          c.rwsize -= len;
          sendit = true;
        }
      }
      if (sendit) {
        _write(packet);
        if (length == 0) {
          return;
        }
        packet.unshift(command, recipient, s, length);
      }

      synchronized (c) {
        if (in_kex) {
          continue;
        }
        if (c.rwsize >= length) {
          c.rwsize -= length;
          break;
        }

        // try{
        // System.out.println("1wait: "+c.rwsize);
        // c.notifyme++;
        // c.wait(100);
        // }
        // catch(InterruptedException e){
        // }
        // finally{
        // c.notifyme--;
        // }
      }
    }
    _write(packet);
  }

  void write(Packet packet) throws Exception {
    // System.err.println("in_kex="+in_kex+" "+(packet.buffer.getCommand()));
    long t = getTimeout();
    while (in_kex) {
      if (t > 0L && (System.currentTimeMillis() - kex_start_time) > t && !in_prompt) {
        throw new JSchException("timeout in waiting for rekeying process.");
      }
      byte command = packet.buffer.getCommand();
      // System.err.println("command: "+command);
      if (command == SSH_MSG_KEXINIT || command == SSH_MSG_NEWKEYS || command == SSH_MSG_KEXDH_INIT
          || command == SSH_MSG_KEXDH_REPLY || command == SSH_MSG_KEX_DH_GEX_GROUP
          || command == SSH_MSG_KEX_DH_GEX_INIT || command == SSH_MSG_KEX_DH_GEX_REPLY
          || command == SSH_MSG_KEX_DH_GEX_REQUEST || command == SSH_MSG_DISCONNECT) {
        break;
      }
      try {
        Thread.sleep(10);
      } catch (InterruptedException e) {
      } ;
    }
    _write(packet);
  }

  private void _write(Packet packet) throws Exception {
    boolean initialKex = this.initialKex;
    boolean doStrictKex = this.doStrictKex;
    boolean enable_strict_kex = this.enable_strict_kex;
    boolean require_strict_kex = this.require_strict_kex;
    boolean resetSeqo = packet.buffer.getCommand() == SSH_MSG_NEWKEYS && doStrictKex;

    synchronized (lock) {
      encode(packet);
      if (io != null) {
        io.put(packet);
        if (++seqo == 0 && (enable_strict_kex || require_strict_kex) && initialKex) {
          throw new JSchStrictKexException("outgoing sequence number wrapped during initial KEX");
        }
        if (resetSeqo) {
          seqo = 0;
        }
      }
    }

    if (resetSeqo && io != null && getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO,
          "Reset outgoing sequence number after sending SSH_MSG_NEWKEYS for strict KEX");
    }
  }

  Runnable thread;

  void run() {
    thread = this::run;

    byte[] foo;
    Buffer buf = new Buffer();
    Packet packet = new Packet(buf);
    int i = 0;
    Channel channel;
    int[] start = new int[1];
    int[] length = new int[1];
    KeyExchange kex = null;

    int stimeout = 0;
    try {
      while (isConnected && thread != null) {
        try {
          buf = read(buf);
          stimeout = 0;
        } catch (InterruptedIOException /* SocketTimeoutException */ ee) {
          if (!in_kex && stimeout < serverAliveCountMax) {
            sendKeepAliveMsg();
            stimeout++;
            continue;
          } else if (in_kex && stimeout < serverAliveCountMax) {
            stimeout++;
            continue;
          }
          throw ee;
        }

        int msgType = buf.getCommand() & 0xff;

        if (kex != null && kex.getState() == msgType) {
          kex_start_time = System.currentTimeMillis();
          boolean result = kex.next(buf);
          if (!result) {
            throw new JSchException("verify: " + result);
          }
          continue;
        }

        switch (msgType) {
          case SSH_MSG_KEXINIT:
            // System.err.println("KEXINIT");
            kex = receive_kexinit(buf);
            break;

          case SSH_MSG_NEWKEYS:
            // System.err.println("NEWKEYS");
            send_newkeys();
            receive_newkeys(buf, kex);
            kex = null;
            break;

          case SSH_MSG_CHANNEL_DATA:
            buf.getInt();
            buf.getByte();
            buf.getByte();
            i = buf.getInt();
            channel = getChannelById(i);
            foo = buf.getString(start, length);
            if (channel == null) {
              break;
            }

            if (length[0] == 0) {
              break;
            }

            try {
              channel.write(foo, start[0], length[0]);
            } catch (Exception e) {
              // System.err.println(e);
              try {
                channel.disconnect();
              } catch (Exception ee) {
              }
              break;
            }
            int len = length[0];
            channel.setLocalWindowSize(channel.lwsize - len);
            if (channel.lwsize < channel.lwsize_max / 2) {
              packet.reset();
              buf.putByte((byte) SSH_MSG_CHANNEL_WINDOW_ADJUST);
              buf.putInt(channel.getRecipient());
              buf.putInt(channel.lwsize_max - channel.lwsize);
              synchronized (channel) {
                if (!channel.close)
                  write(packet);
              }
              channel.setLocalWindowSize(channel.lwsize_max);
            }
            break;

          case SSH_MSG_CHANNEL_EXTENDED_DATA:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            buf.getInt(); // data_type_code == 1
            foo = buf.getString(start, length);
            // System.err.println("stderr: "+new String(foo,start[0],length[0]));
            if (channel == null) {
              break;
            }

            if (length[0] == 0) {
              break;
            }

            channel.write_ext(foo, start[0], length[0]);

            len = length[0];
            channel.setLocalWindowSize(channel.lwsize - len);
            if (channel.lwsize < channel.lwsize_max / 2) {
              packet.reset();
              buf.putByte((byte) SSH_MSG_CHANNEL_WINDOW_ADJUST);
              buf.putInt(channel.getRecipient());
              buf.putInt(channel.lwsize_max - channel.lwsize);
              synchronized (channel) {
                if (!channel.close)
                  write(packet);
              }
              channel.setLocalWindowSize(channel.lwsize_max);
            }
            break;

          case SSH_MSG_CHANNEL_WINDOW_ADJUST:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            if (channel == null) {
              break;
            }
            channel.addRemoteWindowSize(buf.getUInt());
            break;

          case SSH_MSG_CHANNEL_EOF:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            if (channel != null) {
              // channel.eof_remote=true;
              // channel.eof();
              channel.eof_remote();
            }
            /*
             * packet.reset(); buf.putByte((byte)SSH_MSG_CHANNEL_EOF);
             * buf.putInt(channel.getRecipient()); write(packet);
             */
            break;
          case SSH_MSG_CHANNEL_CLOSE:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            if (channel != null) {
              // channel.close();
              channel.disconnect();
            }
            /*
             * if(Channel.pool.size()==0){ thread=null; }
             */
            break;
          case SSH_MSG_CHANNEL_OPEN_CONFIRMATION:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            int r = buf.getInt();
            long rws = buf.getUInt();
            int rps = buf.getInt();
            if (channel != null) {
              channel.setRemoteWindowSize(rws);
              channel.setRemotePacketSize(rps);
              channel.open_confirmation = true;
              channel.setRecipient(r);
            }
            break;
          case SSH_MSG_CHANNEL_OPEN_FAILURE:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            if (channel != null) {
              int reason_code = buf.getInt();
              // foo=buf.getString(); // additional textual information
              // foo=buf.getString(); // language tag
              channel.setExitStatus(reason_code);
              channel.close = true;
              channel.eof_remote = true;
              channel.setRecipient(0);
            }
            break;
          case SSH_MSG_CHANNEL_REQUEST:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            foo = buf.getString();
            boolean reply = (buf.getByte() != 0);
            channel = getChannelById(i);
            if (channel != null) {
              byte reply_type = (byte) SSH_MSG_CHANNEL_FAILURE;
              String reqType = Util.byte2str(foo);
              if (reqType.equals("exit-status")) {
                i = buf.getInt(); // exit-status
                channel.setExitStatus(i);
                reply_type = (byte) SSH_MSG_CHANNEL_SUCCESS;
              } else if (reqType.equals("exit-signal")) {
                byte[] signal = buf.getString(); // exit-signal
                channel.setExitSignal(Util.byte2str(signal));
                reply_type = (byte) SSH_MSG_CHANNEL_SUCCESS;
              }
              if (reply) {
                packet.reset();
                buf.putByte(reply_type);
                buf.putInt(channel.getRecipient());
                write(packet);
              }
            }
            break;
          case SSH_MSG_CHANNEL_OPEN:
            buf.getInt();
            buf.getShort();
            foo = buf.getString();
            String ctyp = Util.byte2str(foo);
            if (!"forwarded-tcpip".equals(ctyp) && !("x11".equals(ctyp) && x11_forwarding)
                && !("auth-agent@openssh.com".equals(ctyp) && agent_forwarding)) {
              if (getLogger().isEnabled(Logger.DEBUG)) {
                getLogger().log(Logger.DEBUG, "Failed to add channel of type " + ctyp
                    + " - type either unsupported or prohibited");
              }
              packet.reset();
              buf.putByte((byte) SSH_MSG_CHANNEL_OPEN_FAILURE);
              buf.putInt(buf.getInt());
              buf.putInt(Channel.SSH_OPEN_ADMINISTRATIVELY_PROHIBITED);
              buf.putString(Util.empty);
              buf.putString(Util.empty);
              write(packet);
            } else {
              channel = openChannel(ctyp);
              if (channel != null) {
                channel.getData(buf);
                Thread tmp = getThreadFactory().newThread(channel::run);
                tmp.setName("Channel " + ctyp + " " + host);
                if (daemon_thread) {
                  tmp.setDaemon(daemon_thread);
                }
                tmp.start();
              } else {
                if (getLogger().isEnabled(Logger.DEBUG)) {
                  getLogger().log(Logger.DEBUG,
                      "Failed to add channel of type " + ctyp + " - session may be disconnecting");
                }
                packet.reset();
                buf.putByte((byte) SSH_MSG_CHANNEL_OPEN_FAILURE);
                buf.putInt(buf.getInt());
                buf.putInt(Channel.SSH_OPEN_RESOURCE_SHORTAGE);
                buf.putString(Util.empty);
                buf.putString(Util.empty);
                write(packet);
              }
            }
            break;
          case SSH_MSG_CHANNEL_SUCCESS:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            if (channel == null) {
              break;
            }
            channel.reply = 1;
            break;
          case SSH_MSG_CHANNEL_FAILURE:
            buf.getInt();
            buf.getShort();
            i = buf.getInt();
            channel = getChannelById(i);
            if (channel == null) {
              break;
            }
            channel.reply = 0;
            break;
          case SSH_MSG_GLOBAL_REQUEST:
            buf.getInt();
            buf.getShort();
            foo = buf.getString(); // request name
            reply = (buf.getByte() != 0);
            if (reply) {
              packet.reset();
              buf.putByte((byte) SSH_MSG_REQUEST_FAILURE);
              write(packet);
            }
            break;
          case SSH_MSG_REQUEST_FAILURE:
          case SSH_MSG_REQUEST_SUCCESS:
            Thread t = grr.getThread();
            if (t != null) {
              grr.setReply(msgType == SSH_MSG_REQUEST_SUCCESS ? 1 : 0);
              if (msgType == SSH_MSG_REQUEST_SUCCESS && grr.getPort() == 0) {
                buf.getInt();
                buf.getShort();
                grr.setPort(buf.getInt());
              }
              t.interrupt();
            }
            break;
          default:
            // System.err.println("Session.run: unsupported type "+msgType);
            throw new IOException("Unknown SSH message type " + msgType);
        }
      }
    } catch (Exception e) {
      in_kex = false;
      if (getLogger().isEnabled(Logger.INFO)) {
        getLogger().log(Logger.INFO,
            "Caught an exception, leaving main loop due to " + e.getMessage(), e);
      }
      // System.err.println("# Session.run");
      // e.printStackTrace();
    }
    try {
      disconnect();
    } catch (NullPointerException e) {
      // System.err.println("@1");
      // e.printStackTrace();
    } catch (Exception e) {
      // System.err.println("@2");
      // e.printStackTrace();
    }
    isConnected = false;
  }

  void delChannel(Channel c) {
    Lock l = channelsLock.writeLock();
    l.lock();
    try {
      channels.remove(c);
    } finally {
      l.unlock();
    }
  }

  public void disconnect() {
    if (!isConnected)
      return;
    // System.err.println(this+": disconnect");
    // Thread.dumpStack();
    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "Disconnecting from " + host + " port " + port);
    }
    /*
     * for(int i=0; i<Channel.pool.size(); i++){ try{ Channel
     * c=((Channel)(Channel.pool.elementAt(i))); if(c.session==this) c.eof(); } catch(Exception e){
     * } }
     */

    List<Channel> channelsCopy;
    Lock l = channelsLock.writeLock();
    l.lock();
    try {
      disconnectingChannels = true;
      channelsCopy = new ArrayList<>(channels);
    } finally {
      l.unlock();
    }

    for (Channel c : channelsCopy) {
      try {
        c.disconnect();
      } catch (Exception e) {
        if (getLogger().isEnabled(Logger.DEBUG)) {
          getLogger().log(Logger.DEBUG, "Exception disconnecting channel of type "
              + Util.byte2str(c.type) + ": " + e.getMessage(), e);
        }
      }
    }

    isConnected = false;

    PortWatcher.delPort(this);
    ChannelForwardedTCPIP.delPort(this);
    ChannelX11.removeFakedCookie(this);

    synchronized (lock) {
      if (connectThread != null) {
        Thread.yield();
        connectThread.interrupt();
        connectThread = null;
      }
    }
    thread = null;
    try {
      if (io != null) {
        if (io.in != null)
          io.in.close();
        if (io.out != null)
          io.out.close();
        if (io.out_ext != null)
          io.out_ext.close();
      }
      if (proxy == null) {
        if (socket != null)
          socket.close();
      } else {
        synchronized (proxy) {
          proxy.close();
        }
        proxy = null;
      }
    } catch (Exception e) {
      // e.printStackTrace();
    }
    io = null;
    socket = null;
    l.lock();
    try {
      disconnectingChannels = false;
      channels.clear();
    } finally {
      l.unlock();
    }

    // RFC 4253 6.4. the 'sequence_number' is never reset, even if keys/algorithms are renegotiated
    // later. Hence we only reset these on session disconnect as the sequence has to start at zero
    // for the first packet during (re)connect.
    seqi = 0;
    seqo = 0;
    initialKex = true;
    doStrictKex = false;
    doExtInfo = false;
    serverSigAlgs = null;

    // synchronized(jsch.pool){
    // jsch.pool.removeElement(this);
    // }

    jsch.removeSession(this);

    // System.gc();
  }

  /**
   * Sets a thread factory to be used for creating new threads in this instance.
   *
   * @param threadFactory The thread factory to be used; must not be <code>null</code>
   * @throws NullPointerException if the provided thread factory is <code>null</code>
   */
  public void setThreadFactory(ThreadFactory threadFactory) {
    this.threadFactory = Objects.requireNonNull(threadFactory);
  }

  /**
   * Returns the thread factory used by this instance.
   *
   * @return The thread factory associated with this instance. If no specific thread factory has
   *         been set, a default thread factory is returned.
   */
  public ThreadFactory getThreadFactory() {
    return threadFactory;
  }

  /**
   * Registers the local port forwarding for loop-back interface. If <code>lport</code> is
   * <code>0</code>, the tcp port will be allocated.
   *
   * @param lport local port for local port forwarding
   * @param host host address for local port forwarding
   * @param rport remote port number for local port forwarding
   * @return an allocated local TCP port number
   * @see #setPortForwardingL(String bind_address, int lport, String host, int rport,
   *      ServerSocketFactory ssf, int connectTimeout)
   */
  public int setPortForwardingL(int lport, String host, int rport) throws JSchException {
    return setPortForwardingL("127.0.0.1", lport, host, rport);
  }

  /**
   * Registers the local port forwarding. If <code>bind_address</code> is an empty string or '*',
   * the port should be available from all interfaces. If <code>bind_address</code> is
   * <code>"localhost"</code> or <code>null</code>, the listening port will be bound for local use
   * only. If <code>lport</code> is <code>0</code>, the tcp port will be allocated.
   *
   * @param bind_address bind address for local port forwarding
   * @param lport local port for local port forwarding
   * @param host host address for local port forwarding
   * @param rport remote port number for local port forwarding
   * @return an allocated local TCP port number
   * @see #setPortForwardingL(String bind_address, int lport, String host, int rport,
   *      ServerSocketFactory ssf, int connectTimeout)
   */
  public int setPortForwardingL(String bind_address, int lport, String host, int rport)
      throws JSchException {
    return setPortForwardingL(bind_address, lport, host, rport, null);
  }

  /**
   * Registers the local port forwarding. If <code>bind_address</code> is an empty string or
   * <code>"*"</code>, the port should be available from all interfaces. If
   * <code>bind_address</code> is <code>"localhost"</code> or <code>null</code>, the listening port
   * will be bound for local use only. If <code>lport</code> is <code>0</code>, the tcp port will be
   * allocated.
   *
   * @param bind_address bind address for local port forwarding
   * @param lport local port for local port forwarding
   * @param host host address for local port forwarding
   * @param rport remote port number for local port forwarding
   * @param ssf socket factory
   * @return an allocated local TCP port number
   * @see #setPortForwardingL(String bind_address, int lport, String host, int rport,
   *      ServerSocketFactory ssf, int connectTimeout)
   */
  public int setPortForwardingL(String bind_address, int lport, String host, int rport,
      ServerSocketFactory ssf) throws JSchException {
    return setPortForwardingL(bind_address, lport, host, rport, ssf, 0);
  }

  /**
   * Registers the local port forwarding. If <code>bind_address</code> is an empty string or
   * <code>"*"</code>, the port should be available from all interfaces. If
   * <code>bind_address</code> is <code>"localhost"</code> or <code>null</code>, the listening port
   * will be bound for local use only. If <code>lport</code> is <code>0</code>, the tcp port will be
   * allocated.
   *
   * @param bind_address bind address for local port forwarding
   * @param lport local port for local port forwarding
   * @param host host address for local port forwarding
   * @param rport remote port number for local port forwarding
   * @param ssf socket factory
   * @param connectTimeout timeout for establishing port connection
   * @return an allocated local TCP port number
   */
  public int setPortForwardingL(String bind_address, int lport, String host, int rport,
      ServerSocketFactory ssf, int connectTimeout) throws JSchException {
    PortWatcher pw = PortWatcher.addPort(this, bind_address, lport, host, rport, ssf);
    pw.setConnectTimeout(connectTimeout);
    Thread tmp = getThreadFactory().newThread(pw::run);
    tmp.setName("PortWatcher Thread for " + host);
    if (daemon_thread) {
      tmp.setDaemon(daemon_thread);
    }
    tmp.start();
    return pw.lport;
  }

  public int setSocketForwardingL(String bindAddress, int lport, String socketPath,
      ServerSocketFactory ssf, int connectTimeout) throws JSchException {
    PortWatcher pw = PortWatcher.addSocket(this, bindAddress, lport, socketPath, ssf);
    pw.setConnectTimeout(connectTimeout);
    Thread tmp = getThreadFactory().newThread(pw::run);
    tmp.setName("PortWatcher Thread for " + host);
    if (daemon_thread) {
      tmp.setDaemon(daemon_thread);
    }
    tmp.start();
    return pw.lport;
  }

  /**
   * Cancels the local port forwarding assigned at local TCP port <code>lport</code> on loopback
   * interface.
   *
   * @param lport local TCP port
   */
  public void delPortForwardingL(int lport) throws JSchException {
    delPortForwardingL("127.0.0.1", lport);
  }

  /**
   * Cancels the local port forwarding assigned at local TCP port <code>lport</code> on
   * <code>bind_address</code> interface.
   *
   * @param bind_address bind_address of network interfaces
   * @param lport local TCP port
   */
  public void delPortForwardingL(String bind_address, int lport) throws JSchException {
    PortWatcher.delPort(this, bind_address, lport);
  }

  /**
   * Lists the registered local port forwarding.
   *
   * @return a list of "lport:host:hostport"
   */
  public String[] getPortForwardingL() throws JSchException {
    return PortWatcher.getPortForwarding(this);
  }

  /**
   * Registers the remote port forwarding for the loopback interface of the remote.
   *
   * @param rport remote port
   * @param host host address
   * @param lport local port
   * @see #setPortForwardingR(String bind_address, int rport, String host, int lport, SocketFactory
   *      sf)
   */
  public void setPortForwardingR(int rport, String host, int lport) throws JSchException {
    setPortForwardingR(null, rport, host, lport, (SocketFactory) null);
  }

  /**
   * Registers the remote port forwarding. If <code>bind_address</code> is an empty string or
   * <code>"*"</code>, the port should be available from all interfaces. If
   * <code>bind_address</code> is <code>"localhost"</code> or is not given, the listening port will
   * be bound for local use only. Note that if <code>GatewayPorts</code> is <code>"no"</code> on the
   * remote, <code>"localhost"</code> is always used as a bind_address.
   *
   * @param bind_address bind address
   * @param rport remote port
   * @param host host address
   * @param lport local port
   * @see #setPortForwardingR(String bind_address, int rport, String host, int lport, SocketFactory
   *      sf)
   */
  public void setPortForwardingR(String bind_address, int rport, String host, int lport)
      throws JSchException {
    setPortForwardingR(bind_address, rport, host, lport, (SocketFactory) null);
  }

  /**
   * Registers the remote port forwarding for the loopback interface of the remote.
   *
   * @param rport remote port
   * @param host host address
   * @param lport local port
   * @param sf socket factory
   * @see #setPortForwardingR(String bind_address, int rport, String host, int lport, SocketFactory
   *      sf)
   */
  public void setPortForwardingR(int rport, String host, int lport, SocketFactory sf)
      throws JSchException {
    setPortForwardingR(null, rport, host, lport, sf);
  }

  // TODO: This method should return the integer value as the assigned port.
  /**
   * Registers the remote port forwarding. If <code>bind_address</code> is an empty string or
   * <code>"*"</code>, the port should be available from all interfaces. If
   * <code>bind_address</code> is <code>"localhost"</code> or is not given, the listening port will
   * be bound for local use only. Note that if <code>GatewayPorts</code> is <code>"no"</code> on the
   * remote, <code>"localhost"</code> is always used as a bind_address. If <code>rport</code> is
   * <code>0</code>, the TCP port will be allocated on the remote.
   *
   * @param bind_address bind address
   * @param rport remote port
   * @param host host address
   * @param lport local port
   * @param sf socket factory
   */
  public void setPortForwardingR(String bind_address, int rport, String host, int lport,
      SocketFactory sf) throws JSchException {
    int allocated = _setPortForwardingR(bind_address, rport);
    ChannelForwardedTCPIP.addPort(this, bind_address, rport, allocated, host, lport, sf);
  }

  /**
   * Registers the remote port forwarding for the loopback interface of the remote. The TCP
   * connection to <code>rport</code> on the remote will be forwarded to an instance of the class
   * <code>daemon</code>. The class specified by <code>daemon</code> must implement
   * <code>ForwardedTCPIPDaemon</code>.
   *
   * @param rport remote port
   * @param daemon class name, which implements "ForwardedTCPIPDaemon"
   * @see #setPortForwardingR(String bind_address, int rport, String daemon, Object[] arg)
   */
  public void setPortForwardingR(int rport, String daemon) throws JSchException {
    setPortForwardingR(null, rport, daemon, null);
  }

  /**
   * Registers the remote port forwarding for the loopback interface of the remote. The TCP
   * connection to <code>rport</code> on the remote will be forwarded to an instance of the class
   * <code>daemon</code> with the argument <code>arg</code>. The class specified by
   * <code>daemon</code> must implement <code>ForwardedTCPIPDaemon</code>.
   *
   * @param rport remote port
   * @param daemon class name, which implements "ForwardedTCPIPDaemon"
   * @param arg arguments for "daemon"
   * @see #setPortForwardingR(String bind_address, int rport, String daemon, Object[] arg)
   */
  public void setPortForwardingR(int rport, String daemon, Object[] arg) throws JSchException {
    setPortForwardingR(null, rport, daemon, arg);
  }

  /**
   * Registers the remote port forwarding. If <code>bind_address</code> is an empty string or
   * <code>"*"</code>, the port should be available from all interfaces. If
   * <code>bind_address</code> is <code>"localhost"</code> or is not given, the listening port will
   * be bound for local use only. Note that if <code>GatewayPorts</code> is <code>"no"</code> on the
   * remote, <code>"localhost"</code> is always used as a bind_address. The TCP connection to
   * <code>rport</code> on the remote will be forwarded to an instance of the class
   * <code>daemon</code> with the argument <code>arg</code>. The class specified by
   * <code>daemon</code> must implement <code>ForwardedTCPIPDaemon</code>.
   *
   * @param bind_address bind address
   * @param rport remote port
   * @param daemon class name, which implements "ForwardedTCPIPDaemon"
   * @param arg arguments for "daemon"
   * @see #setPortForwardingR(String bind_address, int rport, String daemon, Object[] arg)
   */
  public void setPortForwardingR(String bind_address, int rport, String daemon, Object[] arg)
      throws JSchException {
    int allocated = _setPortForwardingR(bind_address, rport);
    ChannelForwardedTCPIP.addPort(this, bind_address, rport, allocated, daemon, arg);
  }

  /**
   * Lists the registered remote port forwarding.
   *
   * @return a list of "rport:host:hostport"
   */
  public String[] getPortForwardingR() throws JSchException {
    return ChannelForwardedTCPIP.getPortForwarding(this);
  }

  static class Forwarding {
    String bind_address = null;
    int port = -1;
    String host = null;
    int hostport = -1;
    String socketPath = null;
  }

  /**
   * The given argument may be "[bind_address:]port:host:hostport" or "[bind_address:]port
   * host:hostport", which is from LocalForward command of ~/.ssh/config . Also allows
   * "[bind_address:]port:socketPath" or "[bind_address:]port socketPath" for socket forwarding.
   */
  Forwarding parseForwarding(String conf) throws JSchException {
    String[] tmp = conf.split(" ");
    if (tmp.length > 1) { // "[bind_address:]port host:hostport"
      List<String> foo = new ArrayList<>();
      for (int i = 0; i < tmp.length; i++) {
        if (tmp[i].length() == 0)
          continue;
        foo.add(tmp[i].trim());
      }
      StringBuilder sb = new StringBuilder(); // join
      for (int i = 0; i < foo.size(); i++) {
        sb.append(foo.get(i));
        if (i + 1 < foo.size())
          sb.append(":");
      }
      conf = sb.toString();
    }

    String org = conf;
    Forwarding f = new Forwarding();
    try {
      if (conf.lastIndexOf(":") == -1)
        throw new JSchException("parseForwarding: " + org);
      try {
        f.hostport = Integer.parseInt(conf.substring(conf.lastIndexOf(":") + 1));
        conf = conf.substring(0, conf.lastIndexOf(":"));
        if (conf.lastIndexOf(":") == -1)
          throw new JSchException("parseForwarding: " + org);
        f.host = conf.substring(conf.lastIndexOf(":") + 1);
      } catch (NumberFormatException e) {
        f.socketPath = conf.substring(conf.lastIndexOf(":") + 1);
      }
      conf = conf.substring(0, conf.lastIndexOf(":"));
      if (conf.lastIndexOf(":") != -1) {
        f.port = Integer.parseInt(conf.substring(conf.lastIndexOf(":") + 1));
        conf = conf.substring(0, conf.lastIndexOf(":"));
        if (conf.length() == 0 || conf.equals("*"))
          conf = "0.0.0.0";
        if (conf.equals("localhost"))
          conf = "127.0.0.1";
        f.bind_address = conf;
      } else {
        f.port = Integer.parseInt(conf);
        f.bind_address = "127.0.0.1";
      }
    } catch (NumberFormatException e) {
      throw new JSchException("parseForwarding: " + e.toString(), e);
    }
    return f;
  }

  /**
   * Registers the local port forwarding. The argument should be in the format like
   * "[bind_address:]port:host:hostport". If <code>bind_address</code> is an empty string or
   * <code>"*"</code>, the port should be available from all interfaces. If
   * <code>bind_address</code> is <code>"localhost"</code> or is not given, the listening port will
   * be bound for local use only.
   *
   * @param conf configuration of local port forwarding
   * @return an assigned port number
   * @see #setPortForwardingL(String bind_address, int lport, String host, int rport)
   */
  public int setPortForwardingL(String conf) throws JSchException {
    Forwarding f = parseForwarding(conf);
    return setPortForwardingL(f.bind_address, f.port, f.host, f.hostport);
  }

  /**
   * Registers the remote port forwarding. The argument should be in the format like
   * "[bind_address:]port:host:hostport". If the bind_address is not given, the default is to only
   * bind to loopback addresses. If the bind_address is <code>"*"</code> or an empty string, then
   * the forwarding is requested to listen on all interfaces. Note that if <code>GatewayPorts</code>
   * is <code>"no"</code> on the remote, <code>"localhost"</code> is always used for bind_address.
   * If the specified remote is <code>"0"</code>, the TCP port will be allocated on the remote.
   *
   * @param conf configuration of remote port forwarding
   * @return an allocated TCP port on the remote.
   * @see #setPortForwardingR(String bind_address, int rport, String host, int rport)
   */
  public int setPortForwardingR(String conf) throws JSchException {
    Forwarding f = parseForwarding(conf);
    int allocated = _setPortForwardingR(f.bind_address, f.port);
    ChannelForwardedTCPIP.addPort(this, f.bind_address, f.port, allocated, f.host, f.hostport,
        null);
    return allocated;
  }

  /**
   * Instantiates an instance of stream-forwarder to <code>host</code>:<code>port</code>. Set I/O
   * stream to the given channel, and then invoke Channel#connect() method.
   *
   * @param host remote host, which the given stream will be plugged to.
   * @param port remote port, which the given stream will be plugged to.
   */
  public Channel getStreamForwarder(String host, int port) throws JSchException {
    ChannelDirectTCPIP channel = (ChannelDirectTCPIP) openChannel("direct-tcpip");
    if (channel != null) {
      channel.setHost(host);
      channel.setPort(port);
      return channel;
    } else {
      String msg = "Failed to add DirectTCPIP channel to " + host + ":" + port
          + " - session may be disconnecting";
      if (getLogger().isEnabled(Logger.DEBUG)) {
        getLogger().log(Logger.DEBUG, msg);
      }
      throw new JSchException(msg);
    }
  }

  private static class GlobalRequestReply {
    private Thread thread = null;
    private int reply = -1;
    private int port = 0;

    void setThread(Thread thread) {
      this.thread = thread;
      this.reply = -1;
    }

    Thread getThread() {
      return thread;
    }

    void setReply(int reply) {
      this.reply = reply;
    }

    int getReply() {
      return this.reply;
    }

    int getPort() {
      return this.port;
    }

    void setPort(int port) {
      this.port = port;
    }
  }

  private GlobalRequestReply grr = new GlobalRequestReply();

  private int _setPortForwardingR(String bind_address, int rport) throws JSchException {
    synchronized (grr) {
      Buffer buf = new Buffer(200); // ??
      Packet packet = new Packet(buf);

      String address_to_bind = ChannelForwardedTCPIP.normalize(bind_address);

      grr.setThread(Thread.currentThread());
      grr.setPort(rport);

      try {
        // byte SSH_MSG_GLOBAL_REQUEST 80
        // string "tcpip-forward"
        // boolean want_reply
        // string address_to_bind
        // uint32 port number to bind
        packet.reset();
        buf.putByte((byte) SSH_MSG_GLOBAL_REQUEST);
        buf.putString(Util.str2byte("tcpip-forward"));
        buf.putByte((byte) 1);
        buf.putString(Util.str2byte(address_to_bind));
        buf.putInt(rport);
        write(packet);
      } catch (Exception e) {
        grr.setThread(null);
        throw new JSchException(e.toString(), e);
      }

      int count = 0;
      int reply = grr.getReply();
      while (count < 10 && reply == -1) {
        try {
          Thread.sleep(1000);
        } catch (Exception e) {
        }
        count++;
        reply = grr.getReply();
      }
      grr.setThread(null);
      if (reply != 1) {
        throw new JSchException("remote port forwarding failed for listen port " + rport);
      }
      rport = grr.getPort();
    }
    return rport;
  }

  /**
   * Cancels the remote port forwarding assigned at remote TCP port <code>rport</code>.
   *
   * @param rport remote TCP port
   */
  public void delPortForwardingR(int rport) throws JSchException {
    this.delPortForwardingR(null, rport);
  }

  /**
   * Cancels the remote port forwarding assigned at remote TCP port <code>rport</code> bound on the
   * interface at <code>bind_address</code>.
   *
   * @param bind_address bind address of the interface on the remote
   * @param rport remote TCP port
   */
  public void delPortForwardingR(String bind_address, int rport) throws JSchException {
    ChannelForwardedTCPIP.delPort(this, bind_address, rport);
  }

  private void initDeflater(String method) throws JSchException {
    Compression odeflater = deflater;
    if (method.equals("none")) {
      deflater = null;
      if (odeflater != null) {
        odeflater.end();
      }
      return;
    }
    String foo = getConfig(method);
    if (foo != null) {
      if (method.equals("zlib") || (isAuthed && method.equals("zlib@openssh.com"))) {
        try {
          Class<? extends Compression> c = Class.forName(foo).asSubclass(Compression.class);
          deflater = c.getDeclaredConstructor().newInstance();
          int level = 6;
          try {
            level = Integer.parseInt(getConfig("compression_level"));
          } catch (Exception ee) {
          }
          deflater.init(Compression.DEFLATER, level, this);
        } catch (Exception ee) {
          throw new JSchException(ee.toString(), ee);
          // System.err.println(foo+" isn't accessible.");
        } finally {
          if (odeflater != null) {
            odeflater.end();
          }
        }
      }
    }
  }

  private void initInflater(String method) throws JSchException {
    Compression oinflater = inflater;
    if (method.equals("none")) {
      inflater = null;
      if (oinflater != null) {
        oinflater.end();
      }
      return;
    }
    String foo = getConfig(method);
    if (foo != null) {
      if (method.equals("zlib") || (isAuthed && method.equals("zlib@openssh.com"))) {
        try {
          Class<? extends Compression> c = Class.forName(foo).asSubclass(Compression.class);
          inflater = c.getDeclaredConstructor().newInstance();
          inflater.init(Compression.INFLATER, 0, this);
        } catch (Exception ee) {
          throw new JSchException(ee.toString(), ee);
          // System.err.println(foo+" isn't accessible.");
        } finally {
          if (oinflater != null) {
            oinflater.end();
          }
        }
      }
    }
  }

  private Channel createChannel(String type) {
    Channel channel = null;
    if (type.equals("session")) {
      channel = new ChannelSession();
    }
    if (type.equals("shell")) {
      channel = new ChannelShell();
    }
    if (type.equals("exec")) {
      channel = new ChannelExec();
    }
    if (type.equals("x11")) {
      channel = new ChannelX11();
    }
    if (type.equals("auth-agent@openssh.com")) {
      channel = new ChannelAgentForwarding();
    }
    if (type.equals("direct-tcpip")) {
      channel = new ChannelDirectTCPIP();
    }
    if (type.equals("forwarded-tcpip")) {
      channel = new ChannelForwardedTCPIP();
    }
    if (type.equals("sftp")) {
      ChannelSftp sftp = new ChannelSftp();
      boolean useWriteFlushWorkaround = getConfig("use_sftp_write_flush_workaround").equals("yes");
      sftp.setUseWriteFlushWorkaround(useWriteFlushWorkaround);
      channel = sftp;
    }
    if (type.equals("subsystem")) {
      channel = new ChannelSubsystem();
    }
    if (type.equals("direct-streamlocal@openssh.com")) {
      channel = new ChannelDirectStreamLocal();
    }
    if (channel == null) {
      return null;
    }
    return addChannel(channel);
  }

  private Channel addChannel(Channel channel) {
    Lock l = channelsLock.writeLock();
    l.lock();
    try {
      if (!disconnectingChannels && isConnected) {
        channel.setSession(this);
        channels.add(channel);
        return channel;
      } else {
        return null;
      }
    } finally {
      l.unlock();
    }
  }

  String[] getServerSigAlgs() {
    return serverSigAlgs;
  }

  public void setProxy(Proxy proxy) {
    this.proxy = proxy;
  }

  public void setHost(String host) {
    this.host = host;
  }

  public void setPort(int port) {
    this.port = port;
  }

  void setUserName(String username) {
    this.username = username;
  }

  public void setUserInfo(UserInfo userinfo) {
    this.userinfo = userinfo;
  }

  public UserInfo getUserInfo() {
    return userinfo;
  }

  public void setInputStream(InputStream in) {
    this.in = in;
  }

  public void setOutputStream(OutputStream out) {
    this.out = out;
  }

  public void setX11Host(String host) {
    ChannelX11.setHost(host);
  }

  public void setX11Port(int port) {
    ChannelX11.setPort(port);
  }

  public void setX11Cookie(String cookie) {
    ChannelX11.setCookie(cookie);
  }

  /**
   * @deprecated use #setPassword(byte[] password)
   */
  @Deprecated
  public void setPassword(String password) {
    if (password != null)
      this.password = Util.str2byte(password);
  }

  public void setPassword(byte[] password) {
    if (password != null) {
      this.password = new byte[password.length];
      System.arraycopy(password, 0, this.password, 0, password.length);
    }
  }

  public void setConfig(Properties newconf) {
    Hashtable<String, String> foo = new Hashtable<>();
    for (String key : newconf.stringPropertyNames()) {
      foo.put(key, newconf.getProperty(key));
    }
    setConfig(foo);
  }

  public void setConfig(Hashtable<String, String> newconf) {
    synchronized (lock) {
      if (config == null)
        config = new Hashtable<>();
      for (Enumeration<String> e = newconf.keys(); e.hasMoreElements();) {
        String newkey = e.nextElement();
        String key =
            (newkey.equals("PubkeyAcceptedKeyTypes") ? "PubkeyAcceptedAlgorithms" : newkey);
        String value = newconf.get(newkey);
        config.put(key, value);
      }
    }
  }

  public void setConfig(String key, String value) {
    synchronized (lock) {
      if (config == null) {
        config = new Hashtable<>();
      }
      if (key.equals("PubkeyAcceptedKeyTypes")) {
        config.put("PubkeyAcceptedAlgorithms", value);
      } else {
        config.put(key, value);
      }
    }
  }

  public String getConfig(String key) {
    if (key.equals("PubkeyAcceptedKeyTypes")) {
      key = "PubkeyAcceptedAlgorithms";
    }
    Object foo = null;
    if (config != null) {
      foo = config.get(key);
      if (foo instanceof String)
        return (String) foo;
    }
    foo = JSch.getConfig(key);
    if (foo instanceof String)
      return (String) foo;
    return null;
  }

  public void setSocketFactory(SocketFactory sfactory) {
    socket_factory = sfactory;
  }

  public boolean isConnected() {
    return isConnected;
  }

  public int getTimeout() {
    return timeout;
  }

  public void setTimeout(int timeout) throws JSchException {
    if (socket == null) {
      if (timeout < 0) {
        throw new JSchException("invalid timeout value");
      }
      this.timeout = timeout;
      return;
    }
    try {
      socket.setSoTimeout(timeout);
      this.timeout = timeout;
    } catch (Exception e) {
      throw new JSchException(e.toString(), e);
    }
  }

  public String getServerVersion() {
    return Util.byte2str(V_S);
  }

  public String getClientVersion() {
    return Util.byte2str(V_C);
  }

  public void setClientVersion(String cv) {
    V_C = Util.str2byte(cv);
  }

  public void sendIgnore() throws Exception {
    Buffer buf = new Buffer();
    Packet packet = new Packet(buf);
    packet.reset();
    buf.putByte((byte) SSH_MSG_IGNORE);
    write(packet);
  }

  private static final byte[] keepalivemsg = Util.str2byte("keepalive@jcraft.com");

  public void sendKeepAliveMsg() throws Exception {
    Buffer buf = new Buffer();
    Packet packet = new Packet(buf);
    packet.reset();
    buf.putByte((byte) SSH_MSG_GLOBAL_REQUEST);
    buf.putString(keepalivemsg);
    buf.putByte((byte) 1);
    write(packet);
  }

  private static final byte[] nomoresessions = Util.str2byte("no-more-sessions@openssh.com");

  public void noMoreSessionChannels() throws Exception {
    Buffer buf = new Buffer();
    Packet packet = new Packet(buf);
    packet.reset();
    buf.putByte((byte) SSH_MSG_GLOBAL_REQUEST);
    buf.putString(nomoresessions);
    buf.putByte((byte) 0);
    write(packet);
  }

  private HostKey hostkey = null;

  public HostKey getHostKey() {
    return hostkey;
  }

  public String getHost() {
    return host;
  }

  public String getUserName() {
    return username;
  }

  public int getPort() {
    return port;
  }

  public void setHostKeyAlias(String hostKeyAlias) {
    this.hostKeyAlias = hostKeyAlias;
  }

  public String getHostKeyAlias() {
    return hostKeyAlias;
  }

  /**
   * Sets the interval to send a keep-alive message. If zero is specified, any keep-alive message
   * must not be sent. The default interval is zero.
   *
   * @param interval the specified interval, in milliseconds.
   * @see #getServerAliveInterval()
   */
  public void setServerAliveInterval(int interval) throws JSchException {
    setTimeout(interval);
    this.serverAliveInterval = interval;
  }

  /**
   * Returns setting for the interval to send a keep-alive message.
   *
   * @see #setServerAliveInterval(int)
   */
  public int getServerAliveInterval() {
    return this.serverAliveInterval;
  }

  /**
   * Sets the number of keep-alive messages which may be sent without receiving any messages back
   * from the server. If this threshold is reached while keep-alive messages are being sent, the
   * connection will be disconnected. The default value is one.
   *
   * @param count the specified count
   * @see #getServerAliveCountMax()
   */
  public void setServerAliveCountMax(int count) {
    this.serverAliveCountMax = count;
  }

  /**
   * Returns setting for the threshold to send keep-alive messages.
   *
   * @see #setServerAliveCountMax(int)
   */
  public int getServerAliveCountMax() {
    return this.serverAliveCountMax;
  }

  public void setDaemonThread(boolean enable) {
    this.daemon_thread = enable;
  }

  private String[] checkCiphers(String ciphers) {
    if (ciphers == null || ciphers.length() == 0)
      return null;

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "CheckCiphers: " + ciphers);
    }

    String cipherc2s = getConfig("cipher.c2s");
    String ciphers2c = getConfig("cipher.s2c");

    List<String> result = new ArrayList<>();
    String[] _ciphers = Util.split(ciphers, ",");
    for (int i = 0; i < _ciphers.length; i++) {
      String cipher = _ciphers[i];
      if (ciphers2c.indexOf(cipher) == -1 && cipherc2s.indexOf(cipher) == -1)
        continue;
      if (!checkCipher(getConfig(cipher))) {
        result.add(cipher);
      }
    }
    if (result.size() == 0)
      return null;
    String[] foo = result.toArray(new String[0]);

    if (getLogger().isEnabled(Logger.INFO)) {
      for (int i = 0; i < foo.length; i++) {
        getLogger().log(Logger.INFO, foo[i] + " is not available.");
      }
    }

    return foo;
  }

  static boolean checkCipher(String cipher) {
    try {
      Class<? extends Cipher> c = Class.forName(cipher).asSubclass(Cipher.class);
      Cipher _c = c.getDeclaredConstructor().newInstance();
      _c.init(Cipher.ENCRYPT_MODE, new byte[_c.getBlockSize()], new byte[_c.getIVSize()]);
      return true;
    } catch (Exception | LinkageError e) {
      return false;
    }
  }

  private String[] checkMacs(String macs) {
    if (macs == null || macs.length() == 0)
      return null;

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "CheckMacs: " + macs);
    }

    String macc2s = getConfig("mac.c2s");
    String macs2c = getConfig("mac.s2c");

    List<String> result = new ArrayList<>();
    String[] _macs = Util.split(macs, ",");
    for (int i = 0; i < _macs.length; i++) {
      String mac = _macs[i];
      if (macs2c.indexOf(mac) == -1 && macc2s.indexOf(mac) == -1)
        continue;
      if (!checkMac(getConfig(mac))) {
        result.add(mac);
      }
    }
    if (result.size() == 0)
      return null;
    String[] foo = result.toArray(new String[0]);

    if (getLogger().isEnabled(Logger.INFO)) {
      for (int i = 0; i < foo.length; i++) {
        getLogger().log(Logger.INFO, foo[i] + " is not available.");
      }
    }

    return foo;
  }

  static boolean checkMac(String mac) {
    try {
      Class<? extends MAC> c = Class.forName(mac).asSubclass(MAC.class);
      MAC _c = c.getDeclaredConstructor().newInstance();
      _c.init(new byte[_c.getBlockSize()]);
      return true;
    } catch (Exception | LinkageError e) {
      return false;
    }
  }

  private String[] checkKexes(String kexes) {
    if (kexes == null || kexes.length() == 0)
      return null;

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "CheckKexes: " + kexes);
    }

    String kex = getConfig("kex");

    List<String> result = new ArrayList<>();
    String[] _kexes = Util.split(kexes, ",");
    for (int i = 0; i < _kexes.length; i++) {
      String k = _kexes[i];
      if (kex.indexOf(k) == -1)
        continue;
      if (!checkKex(this, getConfig(k))) {
        result.add(k);
      }
    }
    if (result.size() == 0)
      return null;
    String[] foo = result.toArray(new String[0]);

    if (getLogger().isEnabled(Logger.INFO)) {
      for (int i = 0; i < foo.length; i++) {
        getLogger().log(Logger.INFO, foo[i] + " is not available.");
      }
    }

    return foo;
  }

  static boolean checkKex(Session s, String kex) {
    try {
      Class<? extends KeyExchange> c = Class.forName(kex).asSubclass(KeyExchange.class);
      KeyExchange _c = c.getDeclaredConstructor().newInstance();
      _c.doInit(s, null, null, null, null);
      return true;
    } catch (Exception | LinkageError e) {
      return false;
    }
  }

  private String[] checkSignatures(String sigs) {
    if (sigs == null || sigs.length() == 0)
      return null;

    if (getLogger().isEnabled(Logger.INFO)) {
      getLogger().log(Logger.INFO, "CheckSignatures: " + sigs);
    }

    List<String> result = new ArrayList<>();
    String[] _sigs = Util.split(sigs, ",");
    for (int i = 0; i < _sigs.length; i++) {
      try {
        // Map certificate algorithm names to their base signature algorithm.
        // Certificate algorithms use the same Signature implementations as their base algorithms.
        String sigToCheck = OpenSshCertificateKeyTypes.getBaseKeyType(_sigs[i]);
        Class<? extends Signature> c =
            Class.forName(JSch.getConfig(sigToCheck)).asSubclass(Signature.class);
        final Signature sig = c.getDeclaredConstructor().newInstance();
        sig.init();
      } catch (Exception | LinkageError e) {
        result.add(_sigs[i]);
      }
    }
    if (result.size() == 0)
      return null;
    String[] foo = result.toArray(new String[0]);

    if (getLogger().isEnabled(Logger.INFO)) {
      for (int i = 0; i < foo.length; i++) {
        getLogger().log(Logger.INFO, foo[i] + " is not available.");
      }
    }
    return foo;
  }

  /**
   * Checks if a CA signature algorithm is allowed and available at runtime.
   * <p>
   * This method validates that the given algorithm is:
   * <ol>
   * <li>Listed in the configured {@code ca_signature_algorithms}</li>
   * <li>Not in the list of unavailable signature algorithms (as determined by
   * {@code CheckSignatures})</li>
   * </ol>
   * </p>
   *
   * @param algorithm the CA signature algorithm to check
   * @throws JSchException if the algorithm is not allowed or not available at runtime
   */
  void checkCASignatureAlgorithm(String algorithm) throws JSchException {
    String caSignatureAlgorithms = getConfig("ca_signature_algorithms");
    String[] allowedAlgorithms;
    if (caSignatureAlgorithms == null || caSignatureAlgorithms.isEmpty()) {
      allowedAlgorithms = new String[0];
    } else {
      // Check if algorithm is in the allowed list
      allowedAlgorithms = Util.split(caSignatureAlgorithms, ",");
    }
    boolean isAllowed = false;
    for (String allowed : allowedAlgorithms) {
      if (allowed.equals(algorithm)) {
        isAllowed = true;
        break;
      }
    }

    if (!isAllowed) {
      throw new JSchException("CA signature algorithm '" + algorithm
          + "' is not in the allowed ca_signature_algorithms list: " + caSignatureAlgorithms);
    }

    // Check if the algorithm is in the cached unavailable signatures list.
    // Copy volatile field to local variable to avoid race condition.
    String[] unavailableSigs = not_available_shks;
    if (unavailableSigs != null) {
      for (String unavailable : unavailableSigs) {
        if (unavailable.equals(algorithm)) {
          throw new JSchException(
              "CA signature algorithm '" + algorithm + "' is not available at runtime. "
                  + "This may be due to missing cryptographic provider support "
                  + "(e.g., Ed25519 on Java 8 without Bouncy Castle).");
        }
      }
    }
    // If unavailableSigs is null, either all probed algorithms are available,
    // or the user chose not to probe via CheckSignatures - respect that choice.
  }

  /**
   * Sets the identityRepository, which will be referred in the public key authentication. The
   * default value is <code>null</code>.
   *
   * @param identityRepository
   * @see #getIdentityRepository()
   */
  public void setIdentityRepository(IdentityRepository identityRepository) {
    this.identityRepository = identityRepository;
  }

  /**
   * Gets the identityRepository. If this.identityRepository is <code>null</code>,
   * JSch#getIdentityRepository() will be invoked.
   *
   * @see JSch#getIdentityRepository()
   */
  IdentityRepository getIdentityRepository() {
    if (identityRepository == null)
      return jsch.getIdentityRepository();
    return identityRepository;
  }

  /**
   * Sets the hostkeyRepository, which will be referred in checking host keys.
   *
   * @param hostkeyRepository
   * @see #getHostKeyRepository()
   */
  public void setHostKeyRepository(HostKeyRepository hostkeyRepository) {
    this.hostkeyRepository = hostkeyRepository;
  }

  /**
   * Gets the hostkeyRepository. If this.hostkeyRepository is <code>null</code>,
   * JSch#getHostKeyRepository() will be invoked.
   *
   * @see JSch#getHostKeyRepository()
   */
  public HostKeyRepository getHostKeyRepository() {
    if (hostkeyRepository == null)
      return jsch.getHostKeyRepository();
    return hostkeyRepository;
  }

  /*
   * // setProxyCommand("ssh -l user2 host2 -o 'ProxyCommand ssh user1@host1 nc host2 22' nc %h %p")
   * public void setProxyCommand(String command){ setProxy(new ProxyCommand(command)); }
   *
   * class ProxyCommand implements Proxy { String command; Process p = null; InputStream in = null;
   * OutputStream out = null; ProxyCommand(String command){ this.command = command; } public void
   * connect(SocketFactory socket_factory, String host, int port, int timeout) throws Exception {
   * String _command = command.replace("%h", host); _command = _command.replace("%p", new
   * Integer(port).toString()); p = Runtime.getRuntime().exec(_command); in = p.getInputStream();
   * out = p.getOutputStream(); } public Socket getSocket() { return null; } public InputStream
   * getInputStream() { return in; } public OutputStream getOutputStream() { return out; } public
   * void close() { try{ if(p!=null){ p.getErrorStream().close(); p.getOutputStream().close();
   * p.getInputStream().close(); p.destroy(); p=null; } } catch(IOException e){ } } }
   */

  private void applyConfig() throws JSchException {
    ConfigRepository configRepository = jsch.getConfigRepository();
    if (configRepository == null) {
      return;
    }

    ConfigRepository.Config config = configRepository.getConfig(org_host);

    String value = null;

    if (username == null) {
      value = config.getUser();
      if (value != null)
        username = value;
    }

    value = config.getHostname();
    if (value != null)
      host = value;

    int port = config.getPort();
    if (port != -1)
      this.port = port;

    checkConfig(config, "kex");
    checkConfig(config, "server_host_key");
    checkConfig(config, "prefer_known_host_key_types");
    checkConfig(config, "enable_server_sig_algs");
    checkConfig(config, "enable_ext_info_in_auth");
    checkConfig(config, "enable_strict_kex");
    checkConfig(config, "require_strict_kex");
    checkConfig(config, "enable_pubkey_auth_query");
    checkConfig(config, "try_additional_pubkey_algorithms");
    checkConfig(config, "enable_auth_none");
    checkConfig(config, "use_sftp_write_flush_workaround");

    checkConfig(config, "cipher.c2s");
    checkConfig(config, "cipher.s2c");
    checkConfig(config, "mac.c2s");
    checkConfig(config, "mac.s2c");
    checkConfig(config, "compression.c2s");
    checkConfig(config, "compression.s2c");
    checkConfig(config, "compression_level");

    checkConfig(config, "StrictHostKeyChecking");
    checkConfig(config, "HashKnownHosts");
    checkConfig(config, "PreferredAuthentications");
    checkConfig(config, "PubkeyAcceptedAlgorithms");
    checkConfig(config, "FingerprintHash");
    checkConfig(config, "MaxAuthTries");
    checkConfig(config, "ClearAllForwardings");

    value = config.getValue("HostKeyAlias");
    if (value != null)
      this.setHostKeyAlias(value);

    value = config.getValue("UserKnownHostsFile");
    if (value != null) {
      KnownHosts kh = new KnownHosts(jsch);
      kh.setKnownHosts(value);
      this.setHostKeyRepository(kh);
    }

    String[] values = config.getValues("IdentityFile");
    if (values != null) {
      String[] global = configRepository.getConfig("").getValues("IdentityFile");
      if (global != null) {
        for (int i = 0; i < global.length; i++) {
          jsch.addIdentity(global[i]);
        }
      } else {
        global = new String[0];
      }
      if (values.length - global.length > 0) {
        IdentityRepositoryWrapper ir =
            new IdentityRepositoryWrapper(jsch.getIdentityRepository(), true);
        for (int i = 0; i < values.length; i++) {
          String ifile = values[i];
          for (int j = 0; j < global.length; j++) {
            if (!ifile.equals(global[j]))
              continue;
            ifile = null;
            break;
          }
          if (ifile == null)
            continue;
          Identity identity = IdentityFile.newInstance(ifile, null, jsch.instLogger);
          ir.add(identity);
        }
        this.setIdentityRepository(ir);
      }
    }

    value = config.getValue("ServerAliveInterval");
    if (value != null) {
      try {
        this.setServerAliveInterval(Integer.parseInt(value));
      } catch (NumberFormatException e) {
      }
    }

    value = config.getValue("ConnectTimeout");
    if (value != null) {
      try {
        setTimeout(Integer.parseInt(value));
      } catch (NumberFormatException e) {
      }
    }

    value = config.getValue("MaxAuthTries");
    if (value != null) {
      setConfig("MaxAuthTries", value);
    }

    value = config.getValue("ClearAllForwardings");
    if (value != null) {
      setConfig("ClearAllForwardings", value);
    }
  }

  private void applyConfigChannel(ChannelSession channel) throws JSchException {
    ConfigRepository configRepository = jsch.getConfigRepository();
    if (configRepository == null) {
      return;
    }

    ConfigRepository.Config config = configRepository.getConfig(org_host);

    String value = null;

    value = config.getValue("ForwardAgent");
    if (value != null) {
      channel.setAgentForwarding(value.equals("yes"));
    }

    value = config.getValue("RequestTTY");
    if (value != null) {
      channel.setPty(value.equals("yes"));
    }
  }

  private void requestPortForwarding() throws JSchException {

    if (getConfig("ClearAllForwardings").equals("yes"))
      return;

    ConfigRepository configRepository = jsch.getConfigRepository();
    if (configRepository == null) {
      return;
    }

    ConfigRepository.Config config = configRepository.getConfig(org_host);

    String[] values = config.getValues("LocalForward");
    if (values != null) {
      for (int i = 0; i < values.length; i++) {
        setPortForwardingL(values[i]);
      }
    }

    values = config.getValues("RemoteForward");
    if (values != null) {
      for (int i = 0; i < values.length; i++) {
        setPortForwardingR(values[i]);
      }
    }
  }

  private void checkConfig(ConfigRepository.Config config, String key) {
    String value = config.getValue(key);
    if (value == null && key.equals("PubkeyAcceptedAlgorithms"))
      value = config.getValue("PubkeyAcceptedKeyTypes");
    if (value != null)
      this.setConfig(key, value);
  }

  /**
   * Returns the logger being used by this instance of Session. If no particular logger has been
   * set, the instance logger of the jsch instance is returned this session belongs to.
   *
   * @return The logger
   */
  public Logger getLogger() {
    if (logger != null) {
      return logger;
    }
    return jsch.getInstanceLogger();
  }

  /**
   * Sets the logger being used by this instance of Session
   *
   * @param logger The logger or <code>null</code> if the instance logger of this instance's jsch
   *        instance should be used
   */
  public void setLogger(Logger logger) {
    this.logger = logger;
  }

  int getBufferMargin() {
    int buffer_margin = 32 + // maximum padding length
        32; // margin for deflater; deflater may inflate data

    Cipher _c2scipher = c2scipher;
    MAC _c2smac = c2smac;

    // maximum mac length
    int mac_length = 20;
    if (_c2scipher != null && (_c2scipher.isChaCha20() || _c2scipher.isAEAD())) {
      if (_c2scipher.getTagSize() > mac_length) {
        mac_length = _c2scipher.getTagSize();
      }
    } else if (_c2smac != null) {
      if (_c2smac.getBlockSize() > mac_length) {
        mac_length = _c2smac.getBlockSize();
      }
    }
    buffer_margin += mac_length;

    return buffer_margin;
  }
}
