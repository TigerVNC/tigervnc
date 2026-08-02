package com.jcraft.jsch;

/**
 * Constants for OpenSSH certificate key type identifiers.
 * <p>
 * This class provides a centralized location for all OpenSSH certificate key type strings as
 * defined in the OpenSSH certificate protocol. These constants are used throughout JSch for
 * identifying and handling certificate-based authentication.
 * </p>
 *
 * <p>
 * Certificate key types follow the naming convention: {@code <algorithm>-cert-v01@openssh.com}
 * </p>
 *
 * @see <a href="https://cvsweb.openbsd.org/src/usr.bin/ssh/PROTOCOL.certkeys">OpenSSH Certificate
 *      Protocol</a>
 */
final class OpenSshCertificateKeyTypes {

  /**
   * RSA certificate key type using SHA-1 for the host key signature.
   */
  static final String SSH_RSA_CERT_V01 = "ssh-rsa-cert-v01@openssh.com";

  /**
   * RSA certificate key type using SHA-256 for the host key signature.
   */
  static final String RSA_SHA2_256_CERT_V01 = "rsa-sha2-256-cert-v01@openssh.com";

  /**
   * RSA certificate key type using SHA-512 for the host key signature.
   */
  static final String RSA_SHA2_512_CERT_V01 = "rsa-sha2-512-cert-v01@openssh.com";

  /**
   * DSA (DSS) certificate key type.
   */
  static final String SSH_DSS_CERT_V01 = "ssh-dss-cert-v01@openssh.com";

  /**
   * ECDSA certificate key type using NIST P-256 curve.
   */
  static final String ECDSA_SHA2_NISTP256_CERT_V01 = "ecdsa-sha2-nistp256-cert-v01@openssh.com";

  /**
   * ECDSA certificate key type using NIST P-384 curve.
   */
  static final String ECDSA_SHA2_NISTP384_CERT_V01 = "ecdsa-sha2-nistp384-cert-v01@openssh.com";

  /**
   * ECDSA certificate key type using NIST P-521 curve.
   */
  static final String ECDSA_SHA2_NISTP521_CERT_V01 = "ecdsa-sha2-nistp521-cert-v01@openssh.com";

  /**
   * Ed25519 certificate key type.
   */
  static final String SSH_ED25519_CERT_V01 = "ssh-ed25519-cert-v01@openssh.com";

  /**
   * Ed448 certificate key type.
   */
  static final String SSH_ED448_CERT_V01 = "ssh-ed448-cert-v01@openssh.com";

  /**
   * Suffix used for all OpenSSH certificate key types.
   */
  static final String CERT_SUFFIX = "-cert-v01@openssh.com";

  /**
   * Private constructor to prevent instantiation.
   */
  private OpenSshCertificateKeyTypes() {
    // Utility class - do not instantiate
  }

  /**
   * Checks if the given key type string represents an OpenSSH certificate.
   *
   * @param keyType the key type string to check
   * @return {@code true} if the key type is a supported OpenSSH certificate type, {@code false}
   *         otherwise
   */
  static boolean isCertificateKeyType(String keyType) {
    if (keyType == null) {
      return false;
    }
    switch (keyType) {
      case SSH_RSA_CERT_V01:
      case RSA_SHA2_256_CERT_V01:
      case RSA_SHA2_512_CERT_V01:
      case SSH_DSS_CERT_V01:
      case ECDSA_SHA2_NISTP256_CERT_V01:
      case ECDSA_SHA2_NISTP384_CERT_V01:
      case ECDSA_SHA2_NISTP521_CERT_V01:
      case SSH_ED25519_CERT_V01:
      case SSH_ED448_CERT_V01:
        return true;
      default:
        return false;
    }
  }

  /**
   * Extracts the base key type from a certificate key type.
   * <p>
   * For example, {@code ssh-rsa-cert-v01@openssh.com} returns {@code ssh-rsa}.
   * </p>
   *
   * @param certificateKeyType the certificate key type
   * @return the base key type, or the original string if it's not a certificate type, or
   *         {@code null} if the input is {@code null}, empty or blank
   */
  static String getBaseKeyType(String certificateKeyType) {
    if (certificateKeyType == null || certificateKeyType.isEmpty()) {
      return null;
    }
    if (certificateKeyType.endsWith(CERT_SUFFIX)) {
      return certificateKeyType.substring(0, certificateKeyType.length() - CERT_SUFFIX.length());
    }
    return certificateKeyType;
  }

  /**
   * Returns the certificate key type for a given base signature algorithm.
   * <p>
   * For example, {@code ssh-rsa} returns {@code ssh-rsa-cert-v01@openssh.com}.
   * </p>
   *
   * @param baseAlgorithm the base signature algorithm (e.g., "ssh-rsa", "ssh-ed25519")
   * @return the corresponding certificate key type, or {@code null} if the algorithm is not
   *         recognized or is {@code null}
   */
  static String getCertificateKeyType(String baseAlgorithm) {
    if (baseAlgorithm == null) {
      return null;
    }
    switch (baseAlgorithm) {
      case "ssh-rsa":
        return SSH_RSA_CERT_V01;
      case "rsa-sha2-256":
        return RSA_SHA2_256_CERT_V01;
      case "rsa-sha2-512":
        return RSA_SHA2_512_CERT_V01;
      case "ssh-dss":
        return SSH_DSS_CERT_V01;
      case "ecdsa-sha2-nistp256":
        return ECDSA_SHA2_NISTP256_CERT_V01;
      case "ecdsa-sha2-nistp384":
        return ECDSA_SHA2_NISTP384_CERT_V01;
      case "ecdsa-sha2-nistp521":
        return ECDSA_SHA2_NISTP521_CERT_V01;
      case "ssh-ed25519":
        return SSH_ED25519_CERT_V01;
      case "ssh-ed448":
        return SSH_ED448_CERT_V01;
      default:
        return null;
    }
  }
}
