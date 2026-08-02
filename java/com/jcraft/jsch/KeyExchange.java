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

public abstract class KeyExchange {

  static final int PROPOSAL_KEX_ALGS = 0;
  static final int PROPOSAL_SERVER_HOST_KEY_ALGS = 1;
  static final int PROPOSAL_ENC_ALGS_CTOS = 2;
  static final int PROPOSAL_ENC_ALGS_STOC = 3;
  static final int PROPOSAL_MAC_ALGS_CTOS = 4;
  static final int PROPOSAL_MAC_ALGS_STOC = 5;
  static final int PROPOSAL_COMP_ALGS_CTOS = 6;
  static final int PROPOSAL_COMP_ALGS_STOC = 7;
  static final int PROPOSAL_LANG_CTOS = 8;
  static final int PROPOSAL_LANG_STOC = 9;
  static final int PROPOSAL_MAX = 10;
  static final String[] PROPOSAL_NAMES =
      {"KEX algorithms", "host key algorithms", "ciphers c2s", "ciphers s2c", "MACs c2s",
          "MACs s2c", "compression c2s", "compression s2c", "languages c2s", "languages s2c"};

  // static String kex_algs="diffie-hellman-group-exchange-sha1"+
  // ",diffie-hellman-group1-sha1";

  // static String kex="diffie-hellman-group-exchange-sha1";
  static String kex = "diffie-hellman-group1-sha1";
  static String server_host_key = "ssh-rsa,ssh-dss";
  static String enc_c2s = "blowfish-cbc";
  static String enc_s2c = "blowfish-cbc";
  static String mac_c2s = "hmac-md5"; // hmac-md5,hmac-sha1,hmac-ripemd160,
                                      // hmac-sha1-96,hmac-md5-96
  static String mac_s2c = "hmac-md5";
  // static String comp_c2s="none"; // zlib
  // static String comp_s2c="none";
  static String lang_c2s = "";
  static String lang_s2c = "";

  public static final int STATE_END = 0;

  protected Session session = null;
  protected HASH sha = null;
  protected byte[] K = null;
  protected byte[] H = null;
  protected byte[] K_S = null;
  protected OpenSshCertificate hostKeyCertificate = null;

  OpenSshCertificate getHostKeyCertificate() {
    return hostKeyCertificate;
  }

  public abstract void init(Session session, byte[] V_S, byte[] V_C, byte[] I_S, byte[] I_C)
      throws Exception;

  void doInit(Session session, byte[] V_S, byte[] V_C, byte[] I_S, byte[] I_C) throws Exception {
    this.session = session;
    init(session, V_S, V_C, I_S, I_C);
  }

  public abstract boolean next(Buffer buf) throws Exception;

  public abstract int getState();

  protected final int RSA = 0;
  protected final int DSS = 1;
  protected final int ECDSA = 2;
  protected final int EDDSA = 3;
  private int type = 0;
  private String key_alg_name = "";

  public String getKeyType() {
    if (type == DSS)
      return "DSA";
    if (type == RSA)
      return "RSA";
    if (type == EDDSA)
      return "EDDSA";
    return "ECDSA";
  }

  public String getKeyAlgorithName() {
    return key_alg_name;
  }

  protected static String[] guess(Session session, byte[] I_S, byte[] I_C) throws Exception {
    String[] guess = new String[PROPOSAL_MAX];
    Buffer sb = new Buffer(I_S);
    sb.setOffSet(17);
    Buffer cb = new Buffer(I_C);
    cb.setOffSet(17);

    if (session.getLogger().isEnabled(Logger.INFO)) {
      for (int i = 0; i < PROPOSAL_MAX; i++) {
        session.getLogger().log(Logger.INFO,
            "server proposal: " + PROPOSAL_NAMES[i] + ": " + Util.byte2str(sb.getString()));
      }
      for (int i = 0; i < PROPOSAL_MAX; i++) {
        session.getLogger().log(Logger.INFO,
            "client proposal: " + PROPOSAL_NAMES[i] + ": " + Util.byte2str(cb.getString()));
      }
      sb.setOffSet(17);
      cb.setOffSet(17);
    }

    for (int i = 0; i < PROPOSAL_MAX; i++) {
      byte[] sp = sb.getString(); // server proposal
      byte[] cp = cb.getString(); // client proposal
      int j = 0;
      int k = 0;

      loop: while (j < cp.length) {
        while (j < cp.length && cp[j] != ',')
          j++;
        if (k == j)
          throw new JSchAlgoNegoFailException(i, Util.byte2str(cp), Util.byte2str(sp));
        String algorithm = Util.byte2str(cp, k, j - k);
        int l = 0;
        int m = 0;
        while (l < sp.length) {
          while (l < sp.length && sp[l] != ',')
            l++;
          if (m == l)
            throw new JSchAlgoNegoFailException(i, Util.byte2str(cp), Util.byte2str(sp));
          if (algorithm.equals(Util.byte2str(sp, m, l - m))) {
            guess[i] = algorithm;
            break loop;
          }
          l++;
          m = l;
        }
        j++;
        k = j;
      }
      if (j == 0) {
        guess[i] = "";
      } else if (guess[i] == null) {
        throw new JSchAlgoNegoFailException(i, Util.byte2str(cp), Util.byte2str(sp));
      }
    }

    boolean _s2cAEAD = false;
    boolean _c2sAEAD = false;
    try {
      Class<? extends Cipher> _s2cclazz =
          Class.forName(session.getConfig(guess[PROPOSAL_ENC_ALGS_STOC])).asSubclass(Cipher.class);
      Cipher _s2ccipher = _s2cclazz.getDeclaredConstructor().newInstance();
      _s2cAEAD = _s2ccipher.isAEAD();
      if (_s2cAEAD) {
        guess[PROPOSAL_MAC_ALGS_STOC] = null;
      }

      Class<? extends Cipher> _c2sclazz =
          Class.forName(session.getConfig(guess[PROPOSAL_ENC_ALGS_CTOS])).asSubclass(Cipher.class);
      Cipher _c2scipher = _c2sclazz.getDeclaredConstructor().newInstance();
      _c2sAEAD = _c2scipher.isAEAD();
      if (_c2sAEAD) {
        guess[PROPOSAL_MAC_ALGS_CTOS] = null;
      }
    } catch (Exception | LinkageError e) {
      throw new JSchException(e.toString(), e);
    }

    if (session.getLogger().isEnabled(Logger.INFO)) {
      session.getLogger().log(Logger.INFO, "kex: algorithm: " + guess[PROPOSAL_KEX_ALGS]);
      session.getLogger().log(Logger.INFO,
          "kex: host key algorithm: " + guess[PROPOSAL_SERVER_HOST_KEY_ALGS]);
      session.getLogger().log(Logger.INFO,
          "kex: server->client" + " cipher: " + guess[PROPOSAL_ENC_ALGS_STOC] + " MAC: "
              + (_s2cAEAD ? ("<implicit>") : (guess[PROPOSAL_MAC_ALGS_STOC])) + " compression: "
              + guess[PROPOSAL_COMP_ALGS_STOC]);
      session.getLogger().log(Logger.INFO,
          "kex: client->server" + " cipher: " + guess[PROPOSAL_ENC_ALGS_CTOS] + " MAC: "
              + (_c2sAEAD ? ("<implicit>") : (guess[PROPOSAL_MAC_ALGS_CTOS])) + " compression: "
              + guess[PROPOSAL_COMP_ALGS_CTOS]);
    }

    return guess;
  }

  public String getFingerPrint() {
    HASH hash = null;
    try {
      String _c = session.getConfig("FingerprintHash").toLowerCase(Locale.ROOT);
      Class<? extends HASH> c = Class.forName(session.getConfig(_c)).asSubclass(HASH.class);
      hash = c.getDeclaredConstructor().newInstance();
    } catch (Exception e) {
      if (session.getLogger().isEnabled(Logger.ERROR)) {
        session.getLogger().log(Logger.ERROR, "getFingerPrint: " + e.getMessage(), e);
      }
    }
    return Util.getFingerPrint(hash, getHostKey(), true, false);
  }

  byte[] getK() {
    return K;
  }

  void clearK() {
    Util.bzero(K);
    K = null;
  }

  byte[] getH() {
    return H;
  }

  HASH getHash() {
    return sha;
  }

  byte[] getHostKey() {
    return K_S;
  }

  /*
   * It seems JCE included in Oracle's Java7u6(and later) has suddenly changed its behavior. The
   * secrete generated by KeyAgreement#generateSecret() may start with 0, even if it is a positive
   * value. See https://bugs.openjdk.org/browse/JDK-7146728.
   */
  protected byte[] normalize(byte[] secret) {
    // This should be a timing safe version of the following:
    // if (secret.length > 1 && secret[0] == 0 && (secret[1] & 0x80) == 0) {
    // byte[] tmp = new byte[secret.length - 1];
    // System.arraycopy(secret, 1, tmp, 0, tmp.length);
    // Util.bzero(secret);
    // return normalize(tmp);
    // } else {
    // return secret;
    // }

    int len = secret.length;
    if (len < 2) {
      return secret;
    }

    // secret[0] == 0
    int a = 0;
    int s0 = secret[0] & 0xff;
    for (int i = 0; i < 8; i++) {
      int j = s0 >>> i;
      j &= 0x1;
      a |= j;
    }
    a ^= 0x1;

    // (secret[1..n] & 0x80) == 0 && secret[1..n] != 0
    int offset = 0;
    for (int i = 1; i < len; i++) {
      int j = secret[i] & 0x80;
      j >>>= 7;
      j ^= 0x1;
      a &= j;
      offset += a;
      j = secret[i] & 0x7f;
      for (int k = 0; k < 7; k++) {
        int l = j >>> k;
        l &= 0x1;
        l ^= 0x1;
        a &= l;
      }
    }

    len -= offset;
    // Try to remain timing safe by performing an allocation + copy for leading bytes removed
    byte[] foo = new byte[len];
    byte[] bar = new byte[offset];
    System.arraycopy(secret, 0, bar, 0, offset);
    System.arraycopy(secret, offset, foo, 0, len);
    Util.bzero(secret);
    return foo;
  }

  /**
   * Verifies the cryptographic signature of the SSH key exchange hash.
   * <p>
   * This method performs cryptographic verification that the remote server possesses the private
   * key corresponding to the public key presented during the SSH key exchange. It supports both
   * traditional SSH public keys and OpenSSH certificates.
   * </p>
   *
   * <h3>Public Key vs. Certificate Handling</h3>
   * <p>
   * The method handles two distinct input formats:
   * </p>
   * <ul>
   * <li><b>Plain Public Keys:</b> When {@code alg} is a standard key algorithm (e.g.,
   * {@code "ssh-rsa"}, {@code "ssh-ed25519"}), the {@code K_S} parameter contains the server's
   * public key in SSH wire format. The method parses the key components and verifies the signature
   * directly.</li>
   *
   * <li><b>OpenSSH Certificates:</b> When {@code alg} is a certificate type (e.g.,
   * {@code "ssh-rsa-cert-v01@openssh.com"}), the {@code K_S} parameter contains an OpenSSH
   * certificate structure. The method:
   * <ol>
   * <li>Parses the certificate to extract the embedded public key</li>
   * <li>Validates that the certificate is a host certificate (not a user certificate)</li>
   * <li>Replaces {@code K_S} with the extracted public key</li>
   * <li>Extracts the underlying algorithm name from the public key</li>
   * <li>Stores the certificate for subsequent CA validation</li>
   * <li>Proceeds with signature verification using the extracted public key</li>
   * </ol>
   * </li>
   * </ul>
   *
   * <h3>Two-Stage Verification for Certificates</h3>
   * <p>
   * For OpenSSH certificates, this method performs only the <b>first stage</b> of verification:
   * proving that the server possesses the private key corresponding to the public key embedded in
   * the certificate. The <b>second stage</b> (validating the certificate's CA signature, validity
   * period, principals, and other certificate-specific properties) is performed separately by
   * {@link OpenSshCertificateHostKeyVerifier#checkHostCertificate(Session, OpenSshCertificate)}.
   * </p>
   *
   * <h3>Signature Verification Process</h3>
   * <p>
   * After extracting the public key (either from the plain input or from within a certificate), the
   * method:
   * </p>
   * <ol>
   * <li>Determines the key algorithm (RSA, DSS, ECDSA, or EdDSA)</li>
   * <li>Parses the algorithm-specific public key components from the SSH wire format</li>
   * <li>Instantiates the appropriate signature verification class</li>
   * <li>Verifies that {@code sig_of_H} is a valid signature of the exchange hash {@code H} using
   * the public key</li>
   * </ol>
   *
   * @param alg the server host key algorithm name. This can be either a plain key algorithm (e.g.,
   *        {@code "ssh-rsa"}, {@code "ssh-dss"}, {@code "ecdsa-sha2-nistp256"},
   *        {@code "ssh-ed25519"}) or a certificate type (e.g.,
   *        {@code "ssh-rsa-cert-v01@openssh.com"}, {@code "ssh-ed25519-cert-v01@openssh.com"}). For
   *        certificates, this parameter is internally replaced with the underlying key algorithm
   *        extracted from the certificate.
   * @param K_S the server's public key blob in SSH wire format. For plain keys, this contains the
   *        public key directly. For certificates, this contains the complete OpenSSH certificate
   *        structure, which includes the public key along with additional metadata (CA signature,
   *        principals, validity period, etc.). When a certificate is detected, this reference is
   *        replaced internally with the extracted public key for verification purposes.
   * @param index the starting byte offset within {@code K_S} from which to begin parsing. For plain
   *        keys, this is typically the position after the algorithm string. For certificates, this
   *        is typically {@code 0} (start of the certificate blob), and the offset is recalculated
   *        after extracting the embedded public key.
   * @param sig_of_H the signature bytes to verify. This is the server's signature of the exchange
   *        hash {@code H}, which proves the server possesses the private key. The signature format
   *        is algorithm-specific and includes both the algorithm identifier and the actual
   *        signature data in SSH wire format.
   * @return {@code true} if the signature is cryptographically valid and proves the server
   *         possesses the private key; {@code false} otherwise.
   * @throws JSchException if the algorithm is unsupported, if a certificate is detected but is not
   *         a host certificate (e.g., it's a user certificate), if the signature verification class
   *         cannot be instantiated, or if any other error occurs during verification.
   * @throws Exception if an unexpected error occurs during parsing or cryptographic operations.
   */
  protected boolean verify(String alg, byte[] K_S, int index, byte[] sig_of_H) throws Exception {
    int i, j;

    i = index;
    boolean result = false;

    if (OpenSshCertificateKeyTypes.isCertificateKeyType(alg)) {
      OpenSshCertificate certificate = OpenSshCertificateParser.parse(session.jsch.instLogger, K_S);

      // Certificates used for host authentication MUST have "certificate role" of
      // SSH2_CERT_TYPE_HOST.
      // Other certificate types MUST not be accepted.
      if (!certificate.isHostCertificate()) {
        throw new JSchInvalidHostCertificateException("Rejected certificate '" + certificate.getId()
            + "': user certificate presented for host authentication. " + "Host: " + session.host);
      }
      K_S = certificate.getCertificatePublicKey();
      if (K_S == null) {
        throw new JSchException(
            "Invalid certificate '" + certificate.getId() + "': missing public key");
      }

      // Extract algorithm from certificate public key
      i = 0;
      j = 0;
      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      alg = Util.byte2str(K_S, i, j);
      i += j;

      this.hostKeyCertificate = certificate;
    }

    if (alg.equals("ssh-rsa")) {
      byte[] tmp;
      byte[] ee;
      byte[] n;

      type = RSA;
      key_alg_name = alg;

      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      ee = tmp;
      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      n = tmp;

      SignatureRSA sig = null;
      Buffer buf = new Buffer(sig_of_H);
      String foo = Util.byte2str(buf.getString());
      try {
        Class<? extends SignatureRSA> c =
            Class.forName(session.getConfig(foo)).asSubclass(SignatureRSA.class);
        sig = c.getDeclaredConstructor().newInstance();
        sig.init();
      } catch (Exception e) {
        throw new JSchException(e.toString(), e);
      }
      sig.setPubKey(ee, n);
      sig.update(H);
      result = sig.verify(sig_of_H);

      if (session.getLogger().isEnabled(Logger.INFO)) {
        session.getLogger().log(Logger.INFO, "ssh_rsa_verify: " + foo + " signature " + result);
      }
    } else if (alg.equals("ssh-dss")) {
      byte[] q = null;
      byte[] tmp;
      byte[] p;
      byte[] g;
      byte[] f;

      type = DSS;
      key_alg_name = alg;

      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      p = tmp;
      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      q = tmp;
      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      g = tmp;
      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      f = tmp;

      SignatureDSA sig = null;
      try {
        Class<? extends SignatureDSA> c =
            Class.forName(session.getConfig("signature.dss")).asSubclass(SignatureDSA.class);
        sig = c.getDeclaredConstructor().newInstance();
        sig.init();
      } catch (Exception e) {
        throw new JSchException(e.toString(), e);
      }
      sig.setPubKey(f, p, q, g);
      sig.update(H);
      result = sig.verify(sig_of_H);

      if (session.getLogger().isEnabled(Logger.INFO)) {
        session.getLogger().log(Logger.INFO, "ssh_dss_verify: signature " + result);
      }
    } else if (alg.equals("ecdsa-sha2-nistp256") || alg.equals("ecdsa-sha2-nistp384")
        || alg.equals("ecdsa-sha2-nistp521")) {
      byte[] tmp;
      byte[] r;
      byte[] s;

      // RFC 5656,
      type = ECDSA;
      key_alg_name = alg;

      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;
      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      i++;
      tmp = new byte[(j - 1) / 2];
      System.arraycopy(K_S, i, tmp, 0, tmp.length);
      i += (j - 1) / 2;
      r = tmp;
      tmp = new byte[(j - 1) / 2];
      System.arraycopy(K_S, i, tmp, 0, tmp.length);
      i += (j - 1) / 2;
      s = tmp;

      SignatureECDSA sig = null;
      try {
        Class<? extends SignatureECDSA> c =
            Class.forName(session.getConfig(alg)).asSubclass(SignatureECDSA.class);
        sig = c.getDeclaredConstructor().newInstance();
        sig.init();
      } catch (Exception e) {
        throw new JSchException(e.toString(), e);
      }

      sig.setPubKey(r, s);

      sig.update(H);

      result = sig.verify(sig_of_H);

      if (session.getLogger().isEnabled(Logger.INFO)) {
        session.getLogger().log(Logger.INFO, "ssh_ecdsa_verify: " + alg + " signature " + result);
      }
    } else if (alg.equals("ssh-ed25519") || alg.equals("ssh-ed448")) {
      byte[] tmp;

      // RFC 8709,
      type = EDDSA;
      key_alg_name = alg;

      j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
          | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
      tmp = new byte[j];
      System.arraycopy(K_S, i, tmp, 0, j);
      i += j;

      SignatureEdDSA sig = null;
      try {
        Class<? extends SignatureEdDSA> c =
            Class.forName(session.getConfig(alg)).asSubclass(SignatureEdDSA.class);
        sig = c.getDeclaredConstructor().newInstance();
        sig.init();
      } catch (Exception | LinkageError e) {
        throw new JSchException(e.toString(), e);
      }

      sig.setPubKey(tmp);

      sig.update(H);

      result = sig.verify(sig_of_H);

      if (session.getLogger().isEnabled(Logger.INFO)) {
        session.getLogger().log(Logger.INFO, "ssh_eddsa_verify: " + alg + " signature " + result);
      }
    } else {
      if (session.getLogger().isEnabled(Logger.ERROR)) {
        session.getLogger().log(Logger.ERROR, "unknown alg: " + alg);
      }
    }

    return result;
  }

  protected byte[] encodeInt(int raw) {
    byte[] foo = new byte[4];
    foo[0] = (byte) (raw >>> 24);
    foo[1] = (byte) (raw >>> 16);
    foo[2] = (byte) (raw >>> 8);
    foo[3] = (byte) (raw);
    return foo;
  }

  protected byte[] encodeAsMPInt(byte[] raw) {
    return encodeAsMPInt(raw, true);
  }

  protected byte[] encodeAsMPInt(byte[] raw, boolean bzero) {
    int i = (raw[0] & 0x80) >>> 7;
    int len = raw.length + i;
    byte[] foo = new byte[len + 4];
    // Try to remain timing safe by performing an extra allocation when i == 0
    byte[] bar = new byte[i ^ 0x1];
    foo[0] = (byte) (len >>> 24);
    foo[1] = (byte) (len >>> 16);
    foo[2] = (byte) (len >>> 8);
    foo[3] = (byte) (len);
    System.arraycopy(raw, 0, foo, 4 + i, len - i);
    if (bzero) {
      Util.bzero(raw);
    }
    return foo;
  }

  protected byte[] encodeAsString(byte[] raw) {
    return encodeAsString(raw, true);
  }

  protected byte[] encodeAsString(byte[] raw, boolean bzero) {
    int len = raw.length;
    byte[] foo = new byte[len + 4];
    foo[0] = (byte) (len >>> 24);
    foo[1] = (byte) (len >>> 16);
    foo[2] = (byte) (len >>> 8);
    foo[3] = (byte) (len);
    System.arraycopy(raw, 0, foo, 4, len);
    if (bzero) {
      Util.bzero(raw);
    }
    return foo;
  }
}
