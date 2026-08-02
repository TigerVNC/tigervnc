package com.jcraft.jsch;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * An {@link Identity} implementation that supports OpenSSH certificates.
 *
 * <p>
 * This class handles SSH identity files that contain OpenSSH certificates, which combine a public
 * key with additional metadata and restrictions signed by a certificate authority. It supports all
 * standard OpenSSH certificate types including RSA, DSA, ECDSA, and Ed25519.
 * </p>
 *
 * <p>
 * The class can load certificates from file pairs (private key file + certificate file) and
 * provides all the functionality needed for SSH authentication using certificates.
 * </p>
 */
class OpenSshCertificateAwareIdentityFile implements Identity {

  /**
   * Maximum expected length for a key type string. Used as a sanity check to quickly reject
   * obviously invalid data before performing string conversion. The longest current certificate key
   * type is 41 characters (ecdsa-sha2-nistp521-cert-v01@openssh.com), so 100 provides sufficient
   * headroom for potential future key types while still being small enough for an effective
   * early-exit check.
   */
  static final int MAX_KEY_TYPE_LENGTH = 100;

  /**
   * parsed certificate.
   **/
  private final OpenSshCertificate certificate;

  /**
   * the key type declared in the first part of the file
   **/
  private final String keyType;

  /**
   * The entire certificate as raw bytes
   */
  private final byte[] publicKeyBlob;

  /**
   * The key pair containing the private key
   */
  private final KeyPair kpair;

  /**
   * The identity name/path
   */
  private final String identity;

  /**
   * Optional comment associated with the identity
   */
  private final String comment;

  /**
   * Determines if the given certificate file content as byte array, represents an OpenSSH
   * certificate.
   *
   * @param certificateFileContent the certificate bytes to check
   * @return {@code true} if the file content is a supported OpenSSH certificate type, {@code false}
   *         otherwise
   */
  static boolean isOpenSshCertificateFile(byte[] certificateFileContent) {
    if (certificateFileContent == null || certificateFileContent.length == 0) {
      return false;
    }

    byte[] keyTypeBytes =
        OpenSshCertificateUtil.extractSpaceDelimitedString(certificateFileContent, 0);

    // avoid converting byte array to string if the keyType is clearly not a supported certificate
    if (keyTypeBytes == null || keyTypeBytes.length == 0
        || keyTypeBytes.length > MAX_KEY_TYPE_LENGTH) {
      return false;
    }

    String keyType = new String(keyTypeBytes, StandardCharsets.UTF_8);

    return OpenSshCertificateKeyTypes.isCertificateKeyType(keyType);
  }

  /**
   * Creates a new certificate-aware identity from file paths.
   *
   * @param prvfile path to the private key file
   * @param pubfile path to the certificate file
   * @param instLogger logger instance for debugging
   * @return a new Identity instance
   * @throws JSchException if the files cannot be loaded or parsed
   */
  static Identity newInstance(String prvfile, String pubfile, JSch.InstanceLogger instLogger)
      throws JSchException {
    byte[] prvkey;
    byte[] certificateFileContent;

    try {
      prvkey = Util.fromFile(prvfile);
      certificateFileContent = Util.fromFile(pubfile);
    } catch (IOException e) {
      throw new JSchException(e.toString(), e);
    }
    return newInstance(prvfile, prvkey, certificateFileContent, instLogger);
  }

  /**
   * Creates a new certificate-aware identity from byte arrays.
   *
   * @param name the identity name
   * @param prvkey the private key bytes
   * @param certificateFileContentBytes the certificate bytes
   * @param instLogger logger instance for debugging
   * @return a new Identity instance
   * @throws JSchException if the certificate cannot be parsed
   */
  static Identity newInstance(String name, byte[] prvkey, byte[] certificateFileContentBytes,
      JSch.InstanceLogger instLogger) throws JSchException {
    OpenSshCertificate cert;
    byte[] certPublicKey;
    KeyPair kpair;
    byte[] declaredKeyTypeBytes;
    byte[] commentBytes;
    byte[] keyData;
    String declaredKeyType;
    String comment;

    try {
      declaredKeyTypeBytes = OpenSshCertificateUtil.extractKeyType(certificateFileContentBytes);
      if (declaredKeyTypeBytes == null || declaredKeyTypeBytes.length == 0) {
        throw new JSchException("Invalid certificate file: missing or empty key type");
      }
      byte[] base64KeyDataBytes =
          OpenSshCertificateUtil.extractKeyData(certificateFileContentBytes);
      if (base64KeyDataBytes == null || base64KeyDataBytes.length == 0) {
        throw new JSchException("Invalid certificate file: missing or empty key data");
      }
      commentBytes = OpenSshCertificateUtil.extractComment(certificateFileContentBytes);

      keyData = Util.fromBase64(base64KeyDataBytes, 0, base64KeyDataBytes.length);
      cert = OpenSshCertificateParser.parse(instLogger, keyData);

      declaredKeyType = Util.byte2str(declaredKeyTypeBytes, StandardCharsets.UTF_8);
      comment = commentBytes != null ? Util.byte2str(commentBytes, StandardCharsets.UTF_8) : null;

      // keyType
      if (OpenSshCertificateUtil.isEmpty(cert.getKeyType())
          || !cert.getKeyType().equals(declaredKeyType)) {
        instLogger.getLogger().log(Logger.ERROR,
            "Key type declared at the beginning of the certificate file, does not correspond to the encoded key type. Declared type: '"
                + declaredKeyType + "' - Encoded Key type: '" + cert.getKeyType() + "'");
        throw new JSchException("Certificate key type mismatch: declared type '" + declaredKeyType
            + "' does not match encoded type '" + cert.getKeyType() + "'");
      }

      if (!cert.isValidNow()) {
        instLogger.getLogger().log(Logger.WARN,
            "certificate is not valid. Valid after: "
                + OpenSshCertificateUtil.toDateString(cert.getValidAfter()) + " - Valid before: "
                + OpenSshCertificateUtil.toDateString(cert.getValidBefore()));
      }

      certPublicKey = cert.getCertificatePublicKey();
      if (certPublicKey == null) {
        throw new JSchException("Invalid certificate: missing public key");
      }
      kpair = KeyPair.load(instLogger, prvkey, certPublicKey);

    } catch (IllegalArgumentException e) {
      throw new JSchException("Invalid certificate format: " + e.getMessage(), e);
    } catch (IllegalStateException e) {
      throw new JSchException("Invalid certificate data: " + e.getMessage(), e);
    } catch (RuntimeException e) {
      throw new JSchException("Unexpected error parsing certificate: " + e.getMessage(), e);
    }
    return new OpenSshCertificateAwareIdentityFile(name, declaredKeyType, keyData, cert, kpair,
        comment);
  }

  /**
   * Private constructor for creating certificate-aware identity instances.
   *
   * @param name the identity name
   * @param keyType the key type declared in the certificate file
   * @param publicKeyBlob the decoded public key blob (certificate in binary form)
   * @param certificate the parsed certificate
   * @param kpair the key pair containing the private key
   * @param comment the optional comment from the certificate file
   */
  private OpenSshCertificateAwareIdentityFile(String name, String keyType, byte[] publicKeyBlob,
      OpenSshCertificate certificate, KeyPair kpair, String comment) {
    this.identity = name;
    this.certificate = certificate;
    this.kpair = kpair;
    this.comment = comment;
    this.keyType = keyType;
    this.publicKeyBlob = publicKeyBlob;
  }

  @Override
  public boolean setPassphrase(byte[] passphrase) {
    return kpair.decrypt(passphrase);
  }

  @Override
  public byte[] getPublicKeyBlob() {
    return publicKeyBlob;
  }

  @Override
  public byte[] getSignature(byte[] data) {
    return kpair.getSignature(data);
  }

  @Override
  public byte[] getSignature(byte[] data, String alg) {
    String sigAlg = OpenSshCertificateUtil.getRawKeyType(alg);
    if (sigAlg == null || sigAlg.isEmpty()) {
      sigAlg = OpenSshCertificateUtil.getRawKeyType(keyType);
    }
    return kpair.getSignature(data, sigAlg != null ? sigAlg : keyType);
  }

  @Override
  public String getAlgName() {
    return certificate.getKeyType();
  }

  @Override
  public String getName() {
    return identity;
  }

  @Override
  public boolean isEncrypted() {
    return kpair.isEncrypted();
  }

  @Override
  public void clear() {
    kpair.dispose();
  }

  String getKeyType() {
    return keyType;
  }

  KeyPair getKpair() {
    return kpair;
  }

  String getIdentity() {
    return identity;
  }

  String getComment() {
    return comment;
  }
}
