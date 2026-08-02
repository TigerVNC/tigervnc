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
import java.io.IOException;
import java.io.InputStream;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Map;
import java.util.Vector;

public class JSch {
  /** The version number. */
  public static final String VERSION = Version.getVersion();

  private static final String CERTIFICATE_FILENAME_SUFFIX = "-cert.pub";

  static Hashtable<String, String> config = new Hashtable<>();

  static {
    config.put("kex", Util.getSystemProperty("jsch.kex",
        "mlkem768x25519-sha256,curve25519-sha256,curve25519-sha256@libssh.org,ecdh-sha2-nistp256,ecdh-sha2-nistp384,ecdh-sha2-nistp521,diffie-hellman-group-exchange-sha256,diffie-hellman-group16-sha512,diffie-hellman-group18-sha512,diffie-hellman-group14-sha256"));
    config.put("server_host_key", Util.getSystemProperty("jsch.server_host_key",
        "ssh-ed25519-cert-v01@openssh.com,ecdsa-sha2-nistp256-cert-v01@openssh.com,ecdsa-sha2-nistp384-cert-v01@openssh.com,ecdsa-sha2-nistp521-cert-v01@openssh.com,rsa-sha2-512-cert-v01@openssh.com,rsa-sha2-256-cert-v01@openssh.com,ssh-ed25519,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,rsa-sha2-512,rsa-sha2-256"));
    // CASignatureAlgorithms: specifies which algorithms are allowed for signing of certificates
    // by certificate authorities (CAs). Default matches OpenSSH 8.2+ (excludes ssh-rsa/SHA-1).
    config.put("ca_signature_algorithms", Util.getSystemProperty("jsch.ca_signature_algorithms",
        "ssh-ed25519,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,rsa-sha2-512,rsa-sha2-256"));
    config.put("prefer_known_host_key_types",
        Util.getSystemProperty("jsch.prefer_known_host_key_types", "yes"));
    config.put("enable_strict_kex", Util.getSystemProperty("jsch.enable_strict_kex", "yes"));
    config.put("require_strict_kex", Util.getSystemProperty("jsch.require_strict_kex", "no"));
    config.put("enable_server_sig_algs",
        Util.getSystemProperty("jsch.enable_server_sig_algs", "yes"));
    config.put("enable_ext_info_in_auth",
        Util.getSystemProperty("jsch.enable_ext_info_in_auth", "yes"));
    config.put("cipher.s2c", Util.getSystemProperty("jsch.cipher",
        "aes128-gcm@openssh.com,aes256-gcm@openssh.com,aes128-ctr,aes192-ctr,aes256-ctr"));
    config.put("cipher.c2s", Util.getSystemProperty("jsch.cipher",
        "aes128-gcm@openssh.com,aes256-gcm@openssh.com,aes128-ctr,aes192-ctr,aes256-ctr"));
    config.put("mac.s2c", Util.getSystemProperty("jsch.mac",
        "hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,hmac-sha1-etm@openssh.com,hmac-sha2-256,hmac-sha2-512,hmac-sha1"));
    config.put("mac.c2s", Util.getSystemProperty("jsch.mac",
        "hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,hmac-sha1-etm@openssh.com,hmac-sha2-256,hmac-sha2-512,hmac-sha1"));
    config.put("compression.s2c", Util.getSystemProperty("jsch.compression", "none"));
    config.put("compression.c2s", Util.getSystemProperty("jsch.compression", "none"));

    config.put("lang.s2c", Util.getSystemProperty("jsch.lang", ""));
    config.put("lang.c2s", Util.getSystemProperty("jsch.lang", ""));

    config.put("dhgex_min", Util.getSystemProperty("jsch.dhgex_min", "2048"));
    config.put("dhgex_max", Util.getSystemProperty("jsch.dhgex_max", "8192"));
    config.put("dhgex_preferred", Util.getSystemProperty("jsch.dhgex_preferred", "3072"));

    config.put("compression_level", Util.getSystemProperty("jsch.compression_level", "6"));

    config.put("diffie-hellman-group-exchange-sha1", "com.jcraft.jsch.DHGEX1");
    config.put("diffie-hellman-group1-sha1", "com.jcraft.jsch.DHG1");
    config.put("diffie-hellman-group14-sha1", "com.jcraft.jsch.DHG14");
    config.put("diffie-hellman-group-exchange-sha256", "com.jcraft.jsch.DHGEX256");
    config.put("diffie-hellman-group-exchange-sha224@ssh.com", "com.jcraft.jsch.DHGEX224");
    config.put("diffie-hellman-group-exchange-sha384@ssh.com", "com.jcraft.jsch.DHGEX384");
    config.put("diffie-hellman-group-exchange-sha512@ssh.com", "com.jcraft.jsch.DHGEX512");
    config.put("diffie-hellman-group14-sha256", "com.jcraft.jsch.DHG14256");
    config.put("diffie-hellman-group15-sha512", "com.jcraft.jsch.DHG15");
    config.put("diffie-hellman-group16-sha512", "com.jcraft.jsch.DHG16");
    config.put("diffie-hellman-group17-sha512", "com.jcraft.jsch.DHG17");
    config.put("diffie-hellman-group18-sha512", "com.jcraft.jsch.DHG18");
    config.put("diffie-hellman-group14-sha256@ssh.com", "com.jcraft.jsch.DHG14256");
    config.put("diffie-hellman-group14-sha224@ssh.com", "com.jcraft.jsch.DHG14224");
    config.put("diffie-hellman-group15-sha256@ssh.com", "com.jcraft.jsch.DHG15256");
    config.put("diffie-hellman-group15-sha384@ssh.com", "com.jcraft.jsch.DHG15384");
    config.put("diffie-hellman-group16-sha512@ssh.com", "com.jcraft.jsch.DHG16");
    config.put("diffie-hellman-group16-sha384@ssh.com", "com.jcraft.jsch.DHG16384");
    config.put("diffie-hellman-group18-sha512@ssh.com", "com.jcraft.jsch.DHG18");
    config.put("ecdsa-sha2-nistp256", "com.jcraft.jsch.jce.SignatureECDSA256");
    config.put("ecdsa-sha2-nistp384", "com.jcraft.jsch.jce.SignatureECDSA384");
    config.put("ecdsa-sha2-nistp521", "com.jcraft.jsch.jce.SignatureECDSA521");

    config.put("ecdh-sha2-nistp256", "com.jcraft.jsch.DHEC256");
    config.put("ecdh-sha2-nistp384", "com.jcraft.jsch.DHEC384");
    config.put("ecdh-sha2-nistp521", "com.jcraft.jsch.DHEC521");

    config.put("ecdh-sha2-nistp", "com.jcraft.jsch.jce.ECDHN");

    config.put("curve25519-sha256", "com.jcraft.jsch.DH25519");
    config.put("curve25519-sha256@libssh.org", "com.jcraft.jsch.DH25519");
    config.put("curve448-sha512", "com.jcraft.jsch.DH448");
    config.put("mlkem768x25519-sha256", "com.jcraft.jsch.DH25519MLKEM768");
    config.put("mlkem768nistp256-sha256", "com.jcraft.jsch.DHEC256MLKEM768");
    config.put("mlkem1024nistp384-sha384", "com.jcraft.jsch.DHEC384MLKEM1024");
    config.put("sntrup761x25519-sha512", "com.jcraft.jsch.DH25519SNTRUP761");
    config.put("sntrup761x25519-sha512@openssh.com", "com.jcraft.jsch.DH25519SNTRUP761");

    if (JavaVersion.getVersion() >= 24) {
      config.put("mlkem768", "com.jcraft.jsch.jce.MLKEM768");
      config.put("mlkem1024", "com.jcraft.jsch.jce.MLKEM1024");
    } else {
      config.put("mlkem768", "com.jcraft.jsch.bc.MLKEM768");
      config.put("mlkem1024", "com.jcraft.jsch.bc.MLKEM1024");
    }
    config.put("sntrup761", "com.jcraft.jsch.bc.SNTRUP761");

    config.put("dh", "com.jcraft.jsch.jce.DH");
    config.put("3des-cbc", "com.jcraft.jsch.jce.TripleDESCBC");
    config.put("blowfish-cbc", "com.jcraft.jsch.jce.BlowfishCBC");
    config.put("hmac-sha1", "com.jcraft.jsch.jce.HMACSHA1");
    config.put("hmac-sha1-96", "com.jcraft.jsch.jce.HMACSHA196");
    config.put("hmac-sha2-256", "com.jcraft.jsch.jce.HMACSHA256");
    config.put("hmac-sha2-512", "com.jcraft.jsch.jce.HMACSHA512");
    config.put("hmac-md5", "com.jcraft.jsch.jce.HMACMD5");
    config.put("hmac-md5-96", "com.jcraft.jsch.jce.HMACMD596");
    config.put("hmac-sha1-etm@openssh.com", "com.jcraft.jsch.jce.HMACSHA1ETM");
    config.put("hmac-sha1-96-etm@openssh.com", "com.jcraft.jsch.jce.HMACSHA196ETM");
    config.put("hmac-sha2-256-etm@openssh.com", "com.jcraft.jsch.jce.HMACSHA256ETM");
    config.put("hmac-sha2-512-etm@openssh.com", "com.jcraft.jsch.jce.HMACSHA512ETM");
    config.put("hmac-md5-etm@openssh.com", "com.jcraft.jsch.jce.HMACMD5ETM");
    config.put("hmac-md5-96-etm@openssh.com", "com.jcraft.jsch.jce.HMACMD596ETM");
    config.put("hmac-sha256-2@ssh.com", "com.jcraft.jsch.jce.HMACSHA2562SSHCOM");
    config.put("hmac-sha224@ssh.com", "com.jcraft.jsch.jce.HMACSHA224SSHCOM");
    config.put("hmac-sha256@ssh.com", "com.jcraft.jsch.jce.HMACSHA256SSHCOM");
    config.put("hmac-sha384@ssh.com", "com.jcraft.jsch.jce.HMACSHA384SSHCOM");
    config.put("hmac-sha512@ssh.com", "com.jcraft.jsch.jce.HMACSHA512SSHCOM");
    config.put("sha-1", "com.jcraft.jsch.jce.SHA1");
    config.put("sha-224", "com.jcraft.jsch.jce.SHA224");
    config.put("sha-256", "com.jcraft.jsch.jce.SHA256");
    config.put("sha-384", "com.jcraft.jsch.jce.SHA384");
    config.put("sha-512", "com.jcraft.jsch.jce.SHA512");
    config.put("md5", "com.jcraft.jsch.jce.MD5");
    config.put("sha1", "com.jcraft.jsch.jce.SHA1");
    config.put("sha224", "com.jcraft.jsch.jce.SHA224");
    config.put("sha256", "com.jcraft.jsch.jce.SHA256");
    config.put("sha384", "com.jcraft.jsch.jce.SHA384");
    config.put("sha512", "com.jcraft.jsch.jce.SHA512");
    config.put("signature.dss", "com.jcraft.jsch.jce.SignatureDSA");
    config.put("ssh-rsa", "com.jcraft.jsch.jce.SignatureRSA");
    config.put("rsa-sha2-256", "com.jcraft.jsch.jce.SignatureRSASHA256");
    config.put("rsa-sha2-512", "com.jcraft.jsch.jce.SignatureRSASHA512");
    config.put("ssh-rsa-sha224@ssh.com", "com.jcraft.jsch.jce.SignatureRSASHA224SSHCOM");
    config.put("ssh-rsa-sha256@ssh.com", "com.jcraft.jsch.jce.SignatureRSASHA256SSHCOM");
    config.put("ssh-rsa-sha384@ssh.com", "com.jcraft.jsch.jce.SignatureRSASHA384SSHCOM");
    config.put("ssh-rsa-sha512@ssh.com", "com.jcraft.jsch.jce.SignatureRSASHA512SSHCOM");
    config.put("keypairgen.dsa", "com.jcraft.jsch.jce.KeyPairGenDSA");
    config.put("keypairgen.rsa", "com.jcraft.jsch.jce.KeyPairGenRSA");
    config.put("keypairgen.ecdsa", "com.jcraft.jsch.jce.KeyPairGenECDSA");
    config.put("random", "com.jcraft.jsch.jce.Random");

    config.put("hmac-ripemd160", "com.jcraft.jsch.bc.HMACRIPEMD160");
    config.put("hmac-ripemd160@openssh.com", "com.jcraft.jsch.bc.HMACRIPEMD160OpenSSH");
    config.put("hmac-ripemd160-etm@openssh.com", "com.jcraft.jsch.bc.HMACRIPEMD160ETM");

    config.put("none", "com.jcraft.jsch.CipherNone");

    config.put("aes128-gcm@openssh.com", "com.jcraft.jsch.jce.AES128GCM");
    config.put("aes256-gcm@openssh.com", "com.jcraft.jsch.jce.AES256GCM");

    config.put("aes128-cbc", "com.jcraft.jsch.jce.AES128CBC");
    config.put("aes192-cbc", "com.jcraft.jsch.jce.AES192CBC");
    config.put("aes256-cbc", "com.jcraft.jsch.jce.AES256CBC");
    config.put("rijndael-cbc@lysator.liu.se", "com.jcraft.jsch.jce.AES256CBC");

    config.put("chacha20-poly1305@openssh.com", "com.jcraft.jsch.bc.ChaCha20Poly1305");
    config.put("cast128-cbc", "com.jcraft.jsch.bc.CAST128CBC");
    config.put("cast128-ctr", "com.jcraft.jsch.bc.CAST128CTR");
    config.put("twofish128-cbc", "com.jcraft.jsch.bc.Twofish128CBC");
    config.put("twofish192-cbc", "com.jcraft.jsch.bc.Twofish192CBC");
    config.put("twofish256-cbc", "com.jcraft.jsch.bc.Twofish256CBC");
    config.put("twofish-cbc", "com.jcraft.jsch.bc.Twofish256CBC");
    config.put("twofish128-ctr", "com.jcraft.jsch.bc.Twofish128CTR");
    config.put("twofish192-ctr", "com.jcraft.jsch.bc.Twofish192CTR");
    config.put("twofish256-ctr", "com.jcraft.jsch.bc.Twofish256CTR");
    config.put("seed-cbc@ssh.com", "com.jcraft.jsch.bc.SEEDCBC");

    config.put("aes128-ctr", "com.jcraft.jsch.jce.AES128CTR");
    config.put("aes192-ctr", "com.jcraft.jsch.jce.AES192CTR");
    config.put("aes256-ctr", "com.jcraft.jsch.jce.AES256CTR");
    config.put("3des-ctr", "com.jcraft.jsch.jce.TripleDESCTR");
    config.put("blowfish-ctr", "com.jcraft.jsch.jce.BlowfishCTR");
    config.put("arcfour", "com.jcraft.jsch.jce.ARCFOUR");
    config.put("arcfour128", "com.jcraft.jsch.jce.ARCFOUR128");
    config.put("arcfour256", "com.jcraft.jsch.jce.ARCFOUR256");

    config.put("userauth.none", "com.jcraft.jsch.UserAuthNone");
    config.put("userauth.password", "com.jcraft.jsch.UserAuthPassword");
    config.put("userauth.keyboard-interactive", "com.jcraft.jsch.UserAuthKeyboardInteractive");
    config.put("userauth.publickey", "com.jcraft.jsch.UserAuthPublicKey");
    config.put("userauth.gssapi-with-mic", "com.jcraft.jsch.UserAuthGSSAPIWithMIC");
    config.put("gssapi-with-mic.krb5", "com.jcraft.jsch.jgss.GSSContextKrb5");

    config.put("zlib", "com.jcraft.jsch.jzlib.Compression");
    config.put("zlib@openssh.com", "com.jcraft.jsch.jzlib.Compression");

    config.put("pbkdf2", "com.jcraft.jsch.jce.PBKDF2");
    config.put("bcrypt", "com.jcraft.jsch.jbcrypt.JBCrypt");
    config.put("Argon2d", "com.jcraft.jsch.bc.Argon2");
    config.put("Argon2i", "com.jcraft.jsch.bc.Argon2");
    config.put("Argon2id", "com.jcraft.jsch.bc.Argon2");
    config.put("scrypt", "com.jcraft.jsch.bc.SCrypt");

    if (JavaVersion.getVersion() >= 11) {
      config.put("xdh", "com.jcraft.jsch.jce.XDH");
    } else {
      config.put("xdh", "com.jcraft.jsch.bc.XDH");
    }

    if (JavaVersion.getVersion() >= 15) {
      config.put("keypairgen.eddsa", "com.jcraft.jsch.jce.KeyPairGenEdDSA");
      config.put("ssh-ed25519", "com.jcraft.jsch.jce.SignatureEd25519");
      config.put("ssh-ed448", "com.jcraft.jsch.jce.SignatureEd448");
    } else {
      config.put("keypairgen.eddsa", "com.jcraft.jsch.bc.KeyPairGenEdDSA");
      config.put("ssh-ed25519", "com.jcraft.jsch.bc.SignatureEd25519");
      config.put("ssh-ed448", "com.jcraft.jsch.bc.SignatureEd448");
    }
    config.put("keypairgen_fromprivate.eddsa", "com.jcraft.jsch.bc.KeyPairGenEdDSA");

    config.put("StrictHostKeyChecking", "ask");
    config.put("HashKnownHosts", "no");

    config.put("PreferredAuthentications", Util.getSystemProperty("jsch.preferred_authentications",
        "gssapi-with-mic,publickey,keyboard-interactive,password"));
    config.put("PubkeyAcceptedAlgorithms", Util.getSystemProperty("jsch.client_pubkey",
        "ssh-ed25519-cert-v01@openssh.com,ecdsa-sha2-nistp256-cert-v01@openssh.com,ecdsa-sha2-nistp384-cert-v01@openssh.com,ecdsa-sha2-nistp521-cert-v01@openssh.com,rsa-sha2-512-cert-v01@openssh.com,rsa-sha2-256-cert-v01@openssh.com,ssh-ed25519,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521,rsa-sha2-512,rsa-sha2-256"));
    config.put("enable_pubkey_auth_query",
        Util.getSystemProperty("jsch.enable_pubkey_auth_query", "yes"));
    config.put("try_additional_pubkey_algorithms",
        Util.getSystemProperty("jsch.try_additional_pubkey_algorithms", "yes"));
    config.put("enable_auth_none", Util.getSystemProperty("jsch.enable_auth_none", "yes"));
    config.put("use_sftp_write_flush_workaround",
        Util.getSystemProperty("jsch.use_sftp_write_flush_workaround", "yes"));

    config.put("CheckCiphers",
        Util.getSystemProperty("jsch.check_ciphers", "chacha20-poly1305@openssh.com"));
    config.put("CheckMacs", Util.getSystemProperty("jsch.check_macs", ""));
    config.put("CheckKexes", Util.getSystemProperty("jsch.check_kexes",
        "mlkem768x25519-sha256,mlkem768nistp256-sha256,mlkem1024nistp384-sha384,sntrup761x25519-sha512,sntrup761x25519-sha512@openssh.com,curve25519-sha256,curve25519-sha256@libssh.org,curve448-sha512"));
    config.put("CheckSignatures",
        Util.getSystemProperty("jsch.check_signatures", "ssh-ed25519,ssh-ed448"));
    config.put("FingerprintHash", Util.getSystemProperty("jsch.fingerprint_hash", "sha256"));

    config.put("MaxAuthTries", Util.getSystemProperty("jsch.max_auth_tries", "6"));
    config.put("ClearAllForwardings", "no");
    /*
     * host_certificate_to_key_fallback: Controls behavior when host certificate validation fails. -
     * "yes" (default): Fall back to standard public key verification using the certificate's
     * embedded public key. This matches OpenSSH behavior, which always performs this fallback. -
     * "no": Reject connection if certificate validation fails (more secure, but may break existing
     * setups when upgrading to a JSch version with certificate support).
     */
    config.put("host_certificate_to_key_fallback",
        Util.getSystemProperty("jsch.host_certificate_to_key_fallback", "yes"));
  }

  final InstanceLogger instLogger = new InstanceLogger();

  private Vector<Session> sessionPool = new Vector<>();

  private IdentityRepository defaultIdentityRepository = new LocalIdentityRepository(instLogger);

  private IdentityRepository identityRepository = defaultIdentityRepository;

  private ConfigRepository configRepository = null;

  /**
   * Sets the <code>identityRepository</code>, which will be referred in the public key
   * authentication.
   *
   * @param identityRepository if <code>null</code> is given, the default repository, which usually
   *        refers to ~/.ssh/, will be used.
   * @see #getIdentityRepository()
   */
  public synchronized void setIdentityRepository(IdentityRepository identityRepository) {
    if (identityRepository == null) {
      this.identityRepository = defaultIdentityRepository;
    } else {
      this.identityRepository = identityRepository;
    }
  }

  public synchronized IdentityRepository getIdentityRepository() {
    return this.identityRepository;
  }

  public ConfigRepository getConfigRepository() {
    return this.configRepository;
  }

  public void setConfigRepository(ConfigRepository configRepository) {
    this.configRepository = configRepository;
  }

  private HostKeyRepository known_hosts = null;

  static final Logger DEVNULL = new Logger() {
    @Override
    public boolean isEnabled(int level) {
      return false;
    }

    @Override
    public void log(int level, String message) {}
  };
  static Logger logger = DEVNULL;

  public JSch() {}

  /**
   * Instantiates the <code>Session</code> object with <code>host</code>. The user name and port
   * number will be retrieved from ConfigRepository. If user name is not given, the system property
   * "user.name" will be referred.
   *
   * @param host hostname
   * @return the instance of <code>Session</code> class.
   * @throws JSchException if <code>username</code> or <code>host</code> are invalid.
   * @see #getSession(String username, String host, int port)
   * @see Session
   * @see ConfigRepository
   */
  public Session getSession(String host) throws JSchException {
    return getSession(null, host, 22);
  }

  /**
   * Instantiates the <code>Session</code> object with <code>username</code> and <code>host</code>.
   * The TCP port 22 will be used in making the connection. Note that the TCP connection must not be
   * established until Session#connect().
   *
   * @param username user name
   * @param host hostname
   * @return the instance of <code>Session</code> class.
   * @throws JSchException if <code>username</code> or <code>host</code> are invalid.
   * @see #getSession(String username, String host, int port)
   * @see Session
   */
  public Session getSession(String username, String host) throws JSchException {
    return getSession(username, host, 22);
  }

  /**
   * Instantiates the <code>Session</code> object with given <code>username</code>,
   * <code>host</code> and <code>port</code>. Note that the TCP connection must not be established
   * until Session#connect().
   *
   * @param username user name
   * @param host hostname
   * @param port port number
   * @return the instance of <code>Session</code> class.
   * @throws JSchException if <code>username</code> or <code>host</code> are invalid.
   * @see #getSession(String username, String host, int port)
   * @see Session
   */
  public Session getSession(String username, String host, int port) throws JSchException {
    if (host == null) {
      throw new JSchException("host must not be null.");
    }
    Session s = new Session(this, username, host, port);
    return s;
  }

  protected void addSession(Session session) {
    synchronized (sessionPool) {
      sessionPool.addElement(session);
    }
  }

  protected boolean removeSession(Session session) {
    synchronized (sessionPool) {
      return sessionPool.remove(session);
    }
  }

  /**
   * Sets the hostkey repository.
   *
   * @param hkrepo
   * @see HostKeyRepository
   * @see KnownHosts
   */
  public void setHostKeyRepository(HostKeyRepository hkrepo) {
    known_hosts = hkrepo;
  }

  /**
   * Sets the instance of <code>KnownHosts</code>, which refers to <code>filename</code>.
   *
   * @param filename filename of known_hosts file.
   * @throws JSchException if the given filename is invalid.
   * @see KnownHosts
   */
  public void setKnownHosts(String filename) throws JSchException {
    if (known_hosts == null)
      known_hosts = new KnownHosts(this);
    if (known_hosts instanceof KnownHosts) {
      synchronized (known_hosts) {
        ((KnownHosts) known_hosts).setKnownHosts(filename);
      }
    }
  }

  /**
   * Sets the instance of <code>KnownHosts</code> generated with <code>stream</code>.
   *
   * @param stream the instance of InputStream from known_hosts file.
   * @throws JSchException if an I/O error occurs.
   * @see KnownHosts
   */
  public void setKnownHosts(InputStream stream) throws JSchException {
    if (known_hosts == null)
      known_hosts = new KnownHosts(this);
    if (known_hosts instanceof KnownHosts) {
      synchronized (known_hosts) {
        ((KnownHosts) known_hosts).setKnownHosts(stream);
      }
    }
  }

  /**
   * Returns the current hostkey repository. By the default, this method will the instance of
   * <code>KnownHosts</code>.
   *
   * @return current hostkey repository.
   * @see HostKeyRepository
   * @see KnownHosts
   */
  public HostKeyRepository getHostKeyRepository() {
    if (known_hosts == null)
      known_hosts = new KnownHosts(this);
    return known_hosts;
  }

  /**
   * Sets the private key, which will be referred in the public key authentication.
   *
   * @param prvkey filename of the private key.
   * @throws JSchException if <code>prvkey</code> is invalid.
   * @see #addIdentity(String prvkey, byte[] passphrase)
   */
  public void addIdentity(String prvkey) throws JSchException {
    addIdentity(prvkey, (byte[]) null);
  }

  /**
   * Sets the private key, which will be referred in the public key authentication. Before
   * registering it into identityRepository, it will be deciphered with <code>passphrase</code>.
   *
   * @param prvkey filename of the private key.
   * @param passphrase passphrase for <code>prvkey</code>.
   * @throws JSchException if <code>passphrase</code> is not right.
   * @see #addIdentity(String prvkey, byte[] passphrase)
   * @deprecated use #addIdentity(String prvkey, byte[] passphrase)
   */
  @Deprecated
  public void addIdentity(String prvkey, String passphrase) throws JSchException {
    byte[] _passphrase = null;
    if (passphrase != null) {
      _passphrase = Util.str2byte(passphrase);
    }
    addIdentity(prvkey, _passphrase);
    if (_passphrase != null)
      Util.bzero(_passphrase);
  }

  /**
   * Sets the private key, which will be referred in the public key authentication. Before
   * registering it into identityRepository, it will be deciphered with <code>passphrase</code>.
   *
   * @param prvkey filename of the private key.
   * @param passphrase passphrase for <code>prvkey</code>.
   * @throws JSchException if <code>passphrase</code> is not right.
   * @see #addIdentity(String prvkey, String pubkey, byte[] passphrase)
   */
  public void addIdentity(String prvkey, byte[] passphrase) throws JSchException {
    Identity identity = IdentityFile.newInstance(prvkey, null, instLogger);
    addIdentity(identity, passphrase);
  }

  /**
   * Sets the private key, which will be referred in the public key authentication. Before
   * registering it into identityRepository, it will be deciphered with <code>passphrase</code>.
   *
   * @param prvkey filename of the private key.
   * @param pubkey filename of the public key.
   * @param passphrase passphrase for <code>prvkey</code>.
   * @throws JSchException if <code>passphrase</code> is not right.
   */
  public void addIdentity(String prvkey, String pubkey, byte[] passphrase) throws JSchException {
    byte[] pubkeyFileContent = null;
    String pubkeyFile = pubkey;
    Identity identity;

    // If pubkey is null, try to auto-discover certificate file (prvkey + "-cert.pub")
    // This mimics KeyPair.load() behavior which tries prvkey + ".pub"
    if (pubkeyFile == null) {
      String certFile = prvkey + CERTIFICATE_FILENAME_SUFFIX;
      if (new File(certFile).exists()) {
        pubkeyFile = certFile;
      }
    }

    if (pubkeyFile != null) {
      try {
        pubkeyFileContent = Util.fromFile(pubkeyFile);
      } catch (IOException e) {
        // Only throw if pubkey was explicitly provided (not auto-discovered)
        // This matches KeyPair.load() behavior
        if (pubkey != null) {
          throw new JSchException(e.toString(), e);
        }
        // Otherwise, silently ignore and fall through to IdentityFile
      }
    }

    if (pubkeyFileContent != null
        && OpenSshCertificateAwareIdentityFile.isOpenSshCertificateFile(pubkeyFileContent)) {
      identity = OpenSshCertificateAwareIdentityFile.newInstance(prvkey, pubkeyFile, instLogger);
    } else {
      identity = IdentityFile.newInstance(prvkey, pubkey, instLogger);
    }

    addIdentity(identity, passphrase);
  }

  /**
   * Sets the private key, which will be referred in the public key authentication. Before
   * registering it into identityRepository, it will be deciphered with <code>passphrase</code>.
   *
   * @param name name of the identity to be used to retrieve it in the identityRepository.
   * @param prvkey private key in byte array.
   * @param pubkey public key in byte array.
   * @param passphrase passphrase for <code>prvkey</code>.
   */
  public void addIdentity(String name, byte[] prvkey, byte[] pubkey, byte[] passphrase)
      throws JSchException {
    Identity identity;
    if (OpenSshCertificateAwareIdentityFile.isOpenSshCertificateFile(pubkey)) {
      identity = OpenSshCertificateAwareIdentityFile.newInstance(name, prvkey, pubkey, instLogger);
    } else {
      identity = IdentityFile.newInstance(name, prvkey, pubkey, instLogger);
    }
    addIdentity(identity, passphrase);
  }

  /**
   * Sets the private key, which will be referred in the public key authentication. Before
   * registering it into identityRepository, it will be deciphered with <code>passphrase</code>.
   *
   * @param identity private key.
   * @param passphrase passphrase for <code>identity</code>.
   * @throws JSchException if <code>passphrase</code> is not right.
   */
  public void addIdentity(Identity identity, byte[] passphrase) throws JSchException {
    if (passphrase != null) {
      try {
        byte[] goo = new byte[passphrase.length];
        System.arraycopy(passphrase, 0, goo, 0, passphrase.length);
        passphrase = goo;
        if (!identity.setPassphrase(passphrase)) {
          throw new JSchException("Incorrect passphrase provided.");
        }
      } finally {
        Util.bzero(passphrase);
      }
    }

    if (identityRepository instanceof LocalIdentityRepository) {
      ((LocalIdentityRepository) identityRepository).add(identity);
    } else if (identity instanceof IdentityFile && !identity.isEncrypted()) {
      identityRepository.add(((IdentityFile) identity).getKeyPair().forSSHAgent());
    } else {
      synchronized (this) {
        if (!(identityRepository instanceof IdentityRepositoryWrapper)) {
          setIdentityRepository(new IdentityRepositoryWrapper(identityRepository));
        }
      }
      ((IdentityRepositoryWrapper) identityRepository).add(identity);
    }
  }

  /**
   * @deprecated use #removeIdentity(Identity identity)
   */
  @Deprecated
  public void removeIdentity(String name) throws JSchException {
    Vector<Identity> identities = identityRepository.getIdentities();
    for (int i = 0; i < identities.size(); i++) {
      Identity identity = identities.elementAt(i);
      if (!identity.getName().equals(name))
        continue;
      if (identityRepository instanceof LocalIdentityRepository) {
        ((LocalIdentityRepository) identityRepository).remove(identity);
      } else
        identityRepository.remove(identity.getPublicKeyBlob());
    }
  }

  /**
   * Removes the identity from identityRepository.
   *
   * @param identity the indentity to be removed.
   * @throws JSchException if <code>identity</code> is invalid.
   */
  public void removeIdentity(Identity identity) throws JSchException {
    identityRepository.remove(identity.getPublicKeyBlob());
  }

  /**
   * Lists names of identities included in the identityRepository.
   *
   * @return names of identities
   * @throws JSchException if identityReposory has problems.
   */
  public Vector<String> getIdentityNames() throws JSchException {
    Vector<String> foo = new Vector<>();
    Vector<Identity> identities = identityRepository.getIdentities();
    for (int i = 0; i < identities.size(); i++) {
      Identity identity = identities.elementAt(i);
      foo.addElement(identity.getName());
    }
    return foo;
  }

  /**
   * Removes all identities from identityRepository.
   *
   * @throws JSchException if identityReposory has problems.
   */
  public void removeAllIdentity() throws JSchException {
    identityRepository.removeAll();
  }

  /**
   * Returns the config value for the specified key.
   *
   * @param key key for the configuration.
   * @return config value
   */
  public static String getConfig(String key) {
    synchronized (config) {
      if (key.equals("PubkeyAcceptedKeyTypes")) {
        key = "PubkeyAcceptedAlgorithms";
      }
      return config.get(key);
    }
  }

  /**
   * Sets or Overrides the configuration.
   *
   * @param newconf configurations
   */
  public static void setConfig(Hashtable<String, String> newconf) {
    synchronized (config) {
      for (Enumeration<String> e = newconf.keys(); e.hasMoreElements();) {
        String newkey = e.nextElement();
        String key =
            (newkey.equals("PubkeyAcceptedKeyTypes") ? "PubkeyAcceptedAlgorithms" : newkey);
        config.put(key, newconf.get(newkey));
      }
    }
  }

  /**
   * Sets or Overrides the configuration.
   *
   * @param key key for the configuration
   * @param value value for the configuration
   */
  public static void setConfig(String key, String value) {
    if (key.equals("PubkeyAcceptedKeyTypes")) {
      config.put("PubkeyAcceptedAlgorithms", value);
    } else {
      config.put(key, value);
    }
  }

  /**
   * Gets the configuration
   */
  public static Map<String, String> getConfig() {
    Map<String, String> ret = new HashMap<>();
    synchronized (config) {
      for (Map.Entry<String, String> entry : config.entrySet()) {
        String key = entry.getKey();
        if (key.equals("PubkeyAcceptedKeyTypes")) {
          key = "PubkeyAcceptedAlgorithms";
        }
        ret.put(key, entry.getValue());
      }
    }
    return Collections.unmodifiableMap(ret);
  }

  /**
   * Sets the logger
   *
   * @param logger logger or <code>null</code> if no logging should take place
   * @see Logger
   */
  public static void setLogger(Logger logger) {
    if (logger == null)
      logger = DEVNULL;
    JSch.logger = logger;
  }

  /**
   * Returns a logger to be used for this particular instance of JSch
   *
   * @return The logger that is used by this instance. If no particular logger has been set, the
   *         statically set logger is returned.
   */
  public Logger getInstanceLogger() {
    return instLogger.getLogger();
  }

  /**
   * Sets a logger to be used for this particular instance of JSch
   *
   * @param logger The logger to be used or <code>null</code> if the statically set logger should be
   *        used
   */
  public void setInstanceLogger(Logger logger) {
    instLogger.setLogger(logger);
  }

  /**
   * Returns the statically set logger, i.e. the logger being used by all JSch instances without
   * explicitly set logger.
   *
   * @return The logger
   */
  public static Logger getLogger() {
    return logger;
  }

  static class InstanceLogger {
    private Logger logger;

    private InstanceLogger() {}

    Logger getLogger() {
      if (logger == null) {
        return JSch.logger;
      }
      return logger;
    }

    void setLogger(Logger logger) {
      this.logger = logger;
    }
  }
}
