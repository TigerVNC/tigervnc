package com.jcraft.jsch;

import java.util.Collection;
import java.util.Collections;
import java.util.Map;

/**
 * Represents an OpenSSH certificate containing all the fields defined in the OpenSSH certificate
 * format.
 *
 * <p>
 * OpenSSH certificates are a mechanism for providing cryptographic proof of authorization to access
 * SSH resources. They consist of a key along with identity information and usage restrictions that
 * have been signed by a certificate authority (CA).
 * </p>
 *
 * <p>
 * This class supports both user certificates (for authenticating users to hosts) and host
 * certificates (for authenticating hosts to users).
 * </p>
 *
 * @see <a href="https://datatracker.ietf.org/doc/html/draft-miller-ssh-cert-03">OpenSSH Certificate
 *      Protocol</a>
 */
class OpenSshCertificate {

  /**
   * Certificate type constant for user certificates
   */
  static final int SSH2_CERT_TYPE_USER = 1;

  /**
   * Certificate type constant for host certificates
   */
  static final int SSH2_CERT_TYPE_HOST = 2;

  /**
   * Minimum validity period (epoch start)
   */
  static final long MIN_VALIDITY = 0L;

  /**
   * Maximum validity period (maximum unsigned 64-bit value)
   */
  static final long MAX_VALIDITY = 0xffff_ffff_ffff_ffffL;

  /**
   * The certificate key type (e.g., "ssh-rsa-cert-v01@openssh.com")
   */
  private final String keyType;

  /**
   * Random nonce to make certificates unique
   */
  private final byte[] nonce;

  /**
   * The certificate's key in SSH wire format
   */
  private final byte[] certificatePublicKey;

  /**
   * Certificate serial number
   */
  private final long serial;

  /**
   * Certificate type (user or host)
   */
  private final int type;

  /**
   * Certificate identifier string
   */
  private final String id;

  /**
   * Collection of principal names this certificate is valid for
   */
  private final Collection<String> principals;

  /**
   * match ssh-keygen behavior where the default is the epoch
   */
  private final long validAfter;

  /**
   * match ssh-keygen behavior where the default would be forever
   */
  private final long validBefore;

  /**
   * Critical options that must be recognized by the SSH implementation
   */
  private final Map<String, String> criticalOptions;

  /**
   * Extensions that provide additional functionality
   */
  private final Map<String, String> extensions;

  /**
   * Reserved field for future use
   */
  private final String reserved;

  /**
   * The CA's key that signed this certificate
   */
  private final byte[] signatureKey;

  /**
   * The cryptographic signature of the certificate
   */
  private final byte[] signature;

  /**
   * The certificate data without the certificate type and the signature
   */
  private final byte[] message;

  /**
   * Private constructor to be used exclusively by the Builder.
   */
  private OpenSshCertificate(Builder builder) {
    this.keyType = builder.keyType;
    this.nonce = builder.nonce;
    this.certificatePublicKey = builder.certificatePublicKey;
    this.serial = builder.serial;
    this.type = builder.type;
    this.id = builder.id;
    this.principals = builder.principals;
    this.validAfter = builder.validAfter;
    this.validBefore = builder.validBefore;
    this.criticalOptions = builder.criticalOptions;
    this.extensions = builder.extensions;
    this.reserved = builder.reserved;
    this.signatureKey = builder.signatureKey;
    this.signature = builder.signature;
    this.message = builder.message;
  }

  String getKeyType() {
    return keyType;
  }

  byte[] getNonce() {
    return nonce == null ? null : nonce.clone();
  }

  byte[] getCertificatePublicKey() {
    return certificatePublicKey == null ? null : certificatePublicKey.clone();
  }

  long getSerial() {
    return serial;
  }

  int getType() {
    return type;
  }

  String getId() {
    return id;
  }

  Collection<String> getPrincipals() {
    return principals == null ? null : Collections.unmodifiableCollection(principals);
  }

  long getValidAfter() {
    return validAfter;
  }

  long getValidBefore() {
    return validBefore;
  }

  Map<String, String> getCriticalOptions() {
    return criticalOptions == null ? null : Collections.unmodifiableMap(criticalOptions);
  }

  Map<String, String> getExtensions() {
    return extensions == null ? null : Collections.unmodifiableMap(extensions);
  }

  String getReserved() {
    return reserved;
  }

  byte[] getSignatureKey() {
    return signatureKey == null ? null : signatureKey.clone();
  }

  byte[] getSignature() {
    return signature == null ? null : signature.clone();
  }

  boolean isUserCertificate() {
    return SSH2_CERT_TYPE_USER == type;
  }

  boolean isHostCertificate() {
    return SSH2_CERT_TYPE_HOST == type;
  }

  boolean isValidNow() {
    return OpenSshCertificateUtil.isValidNow(this);
  }

  byte[] getMessage() {
    return message == null ? null : message.clone();
  }

  /**
   * A static inner builder class for creating immutable OpenSshCertificate instances.
   */
  static class Builder {
    private String keyType;
    private byte[] nonce;
    private byte[] certificatePublicKey;
    private long serial;
    private int type;
    private String id;
    private Collection<String> principals;
    private long validAfter = MIN_VALIDITY;
    private long validBefore = MAX_VALIDITY;
    private Map<String, String> criticalOptions;
    private Map<String, String> extensions;
    private String reserved;
    private byte[] signatureKey;
    private byte[] signature;
    private byte[] message;

    Builder() {}

    Builder keyType(String keyType) {
      this.keyType = keyType;
      return this;
    }

    Builder nonce(byte[] nonce) {
      this.nonce = nonce;
      return this;
    }

    Builder certificatePublicKey(byte[] pk) {
      this.certificatePublicKey = pk;
      return this;
    }

    Builder serial(long serial) {
      this.serial = serial;
      return this;
    }

    Builder type(int type) {
      this.type = type;
      return this;
    }

    Builder id(String id) {
      this.id = id;
      return this;
    }

    Builder principals(Collection<String> principals) {
      this.principals = principals;
      return this;
    }

    Builder validAfter(long validAfter) {
      this.validAfter = validAfter;
      return this;
    }

    Builder validBefore(long validBefore) {
      this.validBefore = validBefore;
      return this;
    }

    Builder criticalOptions(Map<String, String> opts) {
      this.criticalOptions = opts;
      return this;
    }

    Builder extensions(Map<String, String> exts) {
      this.extensions = exts;
      return this;
    }

    Builder reserved(String reserved) {
      this.reserved = reserved;
      return this;
    }

    Builder signatureKey(byte[] sigKey) {
      this.signatureKey = sigKey;
      return this;
    }

    Builder signature(byte[] signature) {
      this.signature = signature;
      return this;
    }

    Builder message(byte[] message) {
      this.message = message;
      return this;
    }

    /**
     * Constructs and returns an immutable OpenSshCertificate instance.
     *
     * @return A new, immutable OpenSshCertificate object.
     * @throws IllegalStateException if any required field is missing or invalid.
     */
    OpenSshCertificate build() {
      validate();
      return new OpenSshCertificate(this);
    }

    /**
     * Validates that all required fields are present and valid.
     *
     * @throws IllegalStateException if any required field is missing or invalid.
     */
    private void validate() {
      if (keyType == null || keyType.trim().isEmpty()) {
        throw new IllegalStateException("keyType is required and cannot be null or empty");
      }
      if (nonce == null || nonce.length == 0) {
        throw new IllegalStateException("nonce is required and cannot be null or empty");
      }
      if (certificatePublicKey == null || certificatePublicKey.length == 0) {
        throw new IllegalStateException(
            "certificatePublicKey is required and cannot be null or empty");
      }
      if (type != SSH2_CERT_TYPE_USER && type != SSH2_CERT_TYPE_HOST) {
        throw new IllegalStateException(
            "type must be SSH2_CERT_TYPE_USER (1) or SSH2_CERT_TYPE_HOST (2), got: " + type);
      }
      if (signatureKey == null || signatureKey.length == 0) {
        throw new IllegalStateException("signatureKey is required and cannot be null or empty");
      }
      if (signature == null || signature.length == 0) {
        throw new IllegalStateException("signature is required and cannot be null or empty");
      }
      if (message == null || message.length == 0) {
        throw new IllegalStateException("message is required and cannot be null or empty");
      }
    }
  }
}
