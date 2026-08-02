package com.jcraft.jsch;

import com.jcraft.jsch.JSch.InstanceLogger;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collection;

/**
 * Parser for OpenSSH certificate format.
 *
 * <p>
 * This class is responsible for parsing OpenSSH certificates from their string representation
 * (typically found in .pub files) into structured {@link OpenSshCertificate} objects. It handles
 * the base64 decoding and binary parsing of all certificate fields according to the OpenSSH
 * certificate specification.
 * </p>
 *
 * <p>
 * The parser supports all standard OpenSSH certificate types including RSA, DSA, ECDSA, and Ed25519
 * certificates.
 * </p>
 *
 * @see OpenSshCertificate
 * @see <a href="https://cvsweb.openbsd.org/src/usr.bin/ssh/PROTOCOL.certkeys">OpenSSH Certificate
 *      Protocol</a>
 */
class OpenSshCertificateParser {

  /**
   * Parses the certificate data and returns a complete {@link OpenSshCertificate} object.
   *
   * <p>
   * This method reads all fields from the certificate in the order specified by the OpenSSH
   * certificate format:
   * </p>
   * <ol>
   * <li>Key type</li>
   * <li>Nonce</li>
   * <li>Certificate public key</li>
   * <li>Serial number</li>
   * <li>Certificate type</li>
   * <li>Key ID</li>
   * <li>Valid principals</li>
   * <li>Valid after timestamp</li>
   * <li>Valid before timestamp</li>
   * <li>Critical options</li>
   * <li>Extensions</li>
   * <li>Reserved field</li>
   * <li>Signature key</li>
   * <li>Signature</li>
   * </ol>
   *
   * @param instLogger logger instance for debugging
   * @param certificateData the certificate data
   * @return the parsed certificate object
   * @throws JSchException if the certificate format is invalid or unsupported
   */
  static OpenSshCertificate parse(InstanceLogger instLogger, byte[] certificateData)
      throws JSchException {

    OpenSshCertificateBuffer buffer = new OpenSshCertificateBuffer(certificateData);

    OpenSshCertificate.Builder openSshCertificateBuilder = new OpenSshCertificate.Builder();

    String kTypeFromData = OpenSshCertificateUtil
        .trimToEmptyIfNull(Util.byte2str(buffer.getString(), StandardCharsets.UTF_8));

    openSshCertificateBuilder.keyType(kTypeFromData).nonce(buffer.getString());

    // KeyPair.parsePubkeyBlob expect keytype in public key blob
    KeyPair publicKey = parsePublicKey(instLogger, kTypeFromData, buffer);
    openSshCertificateBuilder.certificatePublicKey(publicKey.getPublicKeyBlob())
        .serial(buffer.getLong()).type(buffer.getInt())
        .id(Util.byte2str(buffer.getString(), StandardCharsets.UTF_8));

    // Principals
    byte[] principalsBlob = buffer.getBytes();
    OpenSshCertificateBuffer principalsBuffer = new OpenSshCertificateBuffer(principalsBlob);
    Collection<String> principals = principalsBuffer.getStrings();
    openSshCertificateBuilder.principals(principals).validAfter(buffer.getLong())
        .validBefore(buffer.getLong()).criticalOptions(buffer.getCriticalOptions())
        .extensions(buffer.getExtensions())
        .reserved(Util.byte2str(buffer.getString(), StandardCharsets.UTF_8))
        .signatureKey(buffer.getString());

    int messageEndIndex = buffer.s;

    byte[] message = Arrays.copyOfRange(buffer.buffer, 0, messageEndIndex);

    openSshCertificateBuilder.message(message);

    openSshCertificateBuilder.signature(buffer.getString());

    OpenSshCertificate certificate = openSshCertificateBuilder.build();

    if (buffer.s != buffer.index) {
      throw new JSchException(
          "Cannot read OpenSSH certificate, got more data than expected: " + buffer.s + ", actual: "
              + buffer.index + ". ID of the ca certificate: " + certificate.getId());
    }

    return certificate;
  }

  /**
   * Parses a public key from a buffer based on the specified key type.
   *
   * This method is used to deserialize public key components from a binary buffer, typically from
   * an SSH certificate or public key file. It uses a {@code switch} statement to handle different
   * key types, including RSA, DSA, ECDSA, Ed25519, and Ed448, and their corresponding certificate
   * variations. The method reads the necessary key components (e.g., modulus, exponent, curve
   * parameters) from the buffer and uses them to construct the appropriate {@link KeyPair} object.
   *
   * @param instLogger An instance of {@link JSch.InstanceLogger} for logging.
   * @param keyType The string identifier for the public key algorithm (e.g.,
   *        "ssh-rsa-cert-v01@openssh.com").
   * @param buffer The {@link Buffer} containing the binary representation of the public key.
   * @return A {@link KeyPair} object representing the parsed public key.
   * @throws JSchException if the key type is unsupported or if there is an error parsing the key
   *         components from the buffer.
   */
  static KeyPair parsePublicKey(InstanceLogger instLogger, String keyType, Buffer buffer)
      throws JSchException {
    switch (keyType) {
      case OpenSshCertificateKeyTypes.SSH_RSA_CERT_V01:
      case OpenSshCertificateKeyTypes.RSA_SHA2_256_CERT_V01:
      case OpenSshCertificateKeyTypes.RSA_SHA2_512_CERT_V01:
        byte[] pub_array = buffer.getMPInt(); // e
        byte[] n_array = buffer.getMPInt(); // n
        return new KeyPairRSA(instLogger, n_array, pub_array, null);
      case OpenSshCertificateKeyTypes.SSH_DSS_CERT_V01:
        byte[] p_array = buffer.getMPInt();
        byte[] q_array = buffer.getMPInt();
        byte[] g_array = buffer.getMPInt();
        byte[] y_array = buffer.getMPInt();
        return new KeyPairDSA(instLogger, p_array, q_array, g_array, y_array, null);
      case OpenSshCertificateKeyTypes.ECDSA_SHA2_NISTP256_CERT_V01:
      case OpenSshCertificateKeyTypes.ECDSA_SHA2_NISTP384_CERT_V01:
      case OpenSshCertificateKeyTypes.ECDSA_SHA2_NISTP521_CERT_V01:
        byte[] name = buffer.getString();
        int len = buffer.getInt();
        int expectedLen = getExpectedEcdsaPointLength(keyType);
        if (len != expectedLen) {
          throw new JSchException("Invalid ECDSA public key length for " + keyType + ": expected "
              + expectedLen + ", got " + len);
        }
        int pointFormat = buffer.getByte();
        if (pointFormat != OpenSshCertificateUtil.EC_POINT_FORMAT_UNCOMPRESSED) {
          throw new JSchException(
              "Invalid ECDSA public key format: expected uncompressed point (0x04), got 0x"
                  + Integer.toHexString(pointFormat & 0xff));
        }
        byte[] r_array = new byte[(len - 1) / 2];
        byte[] s_array = new byte[(len - 1) / 2];
        buffer.getByte(r_array);
        buffer.getByte(s_array);
        return new KeyPairECDSA(instLogger, name, r_array, s_array, null);
      case OpenSshCertificateKeyTypes.SSH_ED25519_CERT_V01:
        int ed25519Len = buffer.getInt();
        if (ed25519Len != OpenSshCertificateUtil.ED25519_PUBLIC_KEY_LENGTH) {
          throw new JSchException("Invalid Ed25519 public key length: expected "
              + OpenSshCertificateUtil.ED25519_PUBLIC_KEY_LENGTH + ", got " + ed25519Len);
        }
        byte[] ed25519_pub_array = new byte[ed25519Len];
        buffer.getByte(ed25519_pub_array);
        return new KeyPairEd25519(instLogger, ed25519_pub_array, null);
      case OpenSshCertificateKeyTypes.SSH_ED448_CERT_V01:
        int ed448Len = buffer.getInt();
        if (ed448Len != OpenSshCertificateUtil.ED448_PUBLIC_KEY_LENGTH) {
          throw new JSchException("Invalid Ed448 public key length: expected "
              + OpenSshCertificateUtil.ED448_PUBLIC_KEY_LENGTH + ", got " + ed448Len);
        }
        byte[] ed448_pub_array = new byte[ed448Len];
        buffer.getByte(ed448_pub_array);
        return new KeyPairEd448(instLogger, ed448_pub_array, null);
      default:
        throw new JSchException("Unsupported Algorithm for Certificate public key: " + keyType);
    }
  }

  /**
   * Returns the expected uncompressed EC point length for a given ECDSA certificate key type.
   *
   * <p>
   * The uncompressed point format consists of a 0x04 prefix byte followed by the X and Y
   * coordinates. The total length is therefore: coordinate_size * 2 + 1.
   * </p>
   *
   * @param keyType the certificate key type
   * @return the expected point length in bytes
   * @throws JSchException if the key type is not a recognized ECDSA certificate type
   */
  private static int getExpectedEcdsaPointLength(String keyType) throws JSchException {
    switch (keyType) {
      case OpenSshCertificateKeyTypes.ECDSA_SHA2_NISTP256_CERT_V01:
        return OpenSshCertificateUtil.ECDSA_P256_POINT_LENGTH;
      case OpenSshCertificateKeyTypes.ECDSA_SHA2_NISTP384_CERT_V01:
        return OpenSshCertificateUtil.ECDSA_P384_POINT_LENGTH;
      case OpenSshCertificateKeyTypes.ECDSA_SHA2_NISTP521_CERT_V01:
        return OpenSshCertificateUtil.ECDSA_P521_POINT_LENGTH;
      default:
        throw new JSchException("Unknown ECDSA certificate key type: " + keyType);
    }
  }
}
