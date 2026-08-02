package com.jcraft.jsch;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

class OpenSshCertificateUtil {

  /**
   * EC point format indicator for uncompressed points (SEC 1, section 2.3.3).
   * <p>
   * In the uncompressed point format, the first byte is 0x04, followed by the X and Y coordinates.
   * This is the standard format used in SSH for ECDSA public keys.
   * </p>
   *
   * @see <a href="https://www.secg.org/sec1-v2.pdf">SEC 1: Elliptic Curve Cryptography</a>
   */
  static final int EC_POINT_FORMAT_UNCOMPRESSED = 0x04;

  /**
   * Expected public key length for Ed25519 keys (32 bytes).
   * <p>
   * Ed25519 uses a 256-bit (32-byte) public key as defined in RFC 8032.
   * </p>
   *
   * @see <a href="https://www.rfc-editor.org/rfc/rfc8032">RFC 8032: Edwards-Curve Digital Signature
   *      Algorithm (EdDSA)</a>
   */
  static final int ED25519_PUBLIC_KEY_LENGTH = 32;

  /**
   * Expected public key length for Ed448 keys (57 bytes).
   * <p>
   * Ed448 uses a 456-bit public key, which requires 57 bytes when encoded, as defined in RFC 8032.
   * </p>
   *
   * @see <a href="https://www.rfc-editor.org/rfc/rfc8032">RFC 8032: Edwards-Curve Digital Signature
   *      Algorithm (EdDSA)</a>
   */
  static final int ED448_PUBLIC_KEY_LENGTH = 57;

  /**
   * Expected uncompressed EC point length for NIST P-256 keys (65 bytes).
   * <p>
   * The uncompressed point format is: 0x04 || X coordinate (32 bytes) || Y coordinate (32 bytes).
   * </p>
   *
   * @see <a href="https://www.rfc-editor.org/rfc/rfc5656">RFC 5656: Elliptic Curve Algorithm
   *      Integration in the Secure Shell Transport Layer</a>
   */
  static final int ECDSA_P256_POINT_LENGTH = 65;

  /**
   * Expected uncompressed EC point length for NIST P-384 keys (97 bytes).
   * <p>
   * The uncompressed point format is: 0x04 || X coordinate (48 bytes) || Y coordinate (48 bytes).
   * </p>
   *
   * @see <a href="https://www.rfc-editor.org/rfc/rfc5656">RFC 5656: Elliptic Curve Algorithm
   *      Integration in the Secure Shell Transport Layer</a>
   */
  static final int ECDSA_P384_POINT_LENGTH = 97;

  /**
   * Expected uncompressed EC point length for NIST P-521 keys (133 bytes).
   * <p>
   * The uncompressed point format is: 0x04 || X coordinate (66 bytes) || Y coordinate (66 bytes).
   * </p>
   *
   * @see <a href="https://www.rfc-editor.org/rfc/rfc5656">RFC 5656: Elliptic Curve Algorithm
   *      Integration in the Secure Shell Transport Layer</a>
   */
  static final int ECDSA_P521_POINT_LENGTH = 133;

  /**
   * Checks whether a {@link HostKey} represents a Certificate Authority (CA) entry in the
   * known_hosts file. A CA entry is identified by the {@code @cert-authority} marker. Such entries
   * designate trusted CAs used to validate OpenSSH certificates presented by hosts during
   * authentication.
   *
   * @param hostKey the {@link HostKey} to test, may be {@code null}
   * @return {@code true} if {@code hostKey} is non-null and carries the {@code @cert-authority}
   *         marker; {@code false} otherwise
   */
  static boolean isKnownHostCaPublicKeyEntry(HostKey hostKey) {
    return Objects.nonNull(hostKey) && "@cert-authority".equals(hostKey.getMarker());
  }

  /**
   * Checks whether a {@link HostKey} is marked as revoked in the known_hosts file, or is
   * {@code null}. It implements fail-closed security semantics by treating {@code null} entries as
   * revoked.
   *
   * @param hostKey the {@link HostKey} to test, may be {@code null}
   * @return {@code true} if {@code hostKey} is {@code null} or carries the {@code @revoked} marker;
   *         {@code false} otherwise
   */
  static boolean isMarkedRevoked(HostKey hostKey) {
    return hostKey == null || "@revoked".equals(hostKey.getMarker());
  }

  /**
   * Trims leading and trailing whitespace from the input string. Returns an empty string if the
   * input is null.
   *
   * @param s the string to trim, may be null
   * @return the trimmed string, or an empty string if the input is null
   */
  static String trimToEmptyIfNull(String s) {
    if (s == null) {
      return "";
    } else {
      return s.trim();
    }
  }

  /**
   * Checks if a String is empty or null.
   *
   * @param string the String to check, may be null
   * @return true if the CharSequence is null or has a length of 0, false otherwise
   */
  static boolean isEmpty(String string) {
    return string == null || string.isEmpty();
  }

  /**
   * Checks if a Collection is empty or null.
   *
   * @param c the Collection to check, may be null
   * @return true if the Collection is null or has 0 elements, false otherwise
   */
  static boolean isEmpty(Collection<?> c) {
    return c == null || c.isEmpty();
  }

  /**
   * Checks if a Map is empty or null.
   *
   * @param c the Map to check, may be null
   * @return true if the Map is null or has 0 elements, false otherwise
   */
  static boolean isEmpty(Map<?, ?> c) {
    return c == null || c.isEmpty();
  }

  /**
   * Extracts the key type from a certificate file content byte array. This method assumes the key
   * type is the first field (index 0) in the space-delimited string.
   *
   * @param certificateFileContent The content of the certificate file as a byte array.
   * @return The key type as a byte array, or {@code null} if the content is null, empty, or the
   *         field does not exist.
   */
  static byte[] extractKeyType(byte[] certificateFileContent) {
    return extractSpaceDelimitedString(certificateFileContent, 0);
  }

  /**
   * Extracts the comment from a certificate file content byte array. This method assumes the
   * comment is the third field (index 2) in the space-delimited string.
   *
   * @param certificateFileContent The content of the certificate file as a byte array.
   * @return The comment as a byte array, or {@code null} if the content is null, empty, or the
   *         field does not exist.
   */
  static byte[] extractComment(byte[] certificateFileContent) {
    return extractSpaceDelimitedString(certificateFileContent, 2);
  }

  /**
   * Extracts the key data from a certificate file content byte array. This method assumes the key
   * data is the second field (index 1) in the space-delimited string.
   *
   * @param certificateFileContent The content of the certificate file as a byte array.
   * @return The key data as a byte array, or {@code null} if the content is null, empty, or the
   *         field does not exist.
   */
  static byte[] extractKeyData(byte[] certificateFileContent) {
    return extractSpaceDelimitedString(certificateFileContent, 1);
  }

  /**
   * Checks if a byte represents a whitespace character (space, tab, newline, or carriage return).
   *
   * @param b the byte to check
   * @return true if the byte is a whitespace character, false otherwise
   */
  private static boolean isWhitespace(byte b) {
    return b == ' ' || b == '\t' || b == '\n' || b == '\r';
  }

  /**
   * A utility method to safely extract a space-delimited field from a certificate content byte
   * array at a given index. This method avoids String allocation by working directly with bytes and
   * returning a byte array. It handles whitespace (space, tab, newline, carriage return) as
   * delimiters.
   *
   * @param certificate The byte array to be parsed, typically representing certificate content.
   * @param index The zero-based index of the field to extract.
   * @return The byte array field at the specified index, or {@code null} if the input is invalid or
   *         the index is out of bounds.
   */
  static byte[] extractSpaceDelimitedString(byte[] certificate, int index) {
    if (certificate == null || certificate.length == 0) {
      return null;
    }
    if (index < 0) {
      return null;
    }

    int fieldCount = 0;
    int fieldStart = -1;
    int i = 0;

    // Skip leading whitespace
    while (i < certificate.length && isWhitespace(certificate[i])) {
      i++;
    }

    while (i < certificate.length) {
      // Found start of a field
      if (!isWhitespace(certificate[i])) {
        if (fieldStart == -1) {
          fieldStart = i;
        }
        i++;
      } else {
        // Found end of a field
        if (fieldStart != -1) {
          if (fieldCount == index) {
            // This is the field we want - copy and return it
            int length = i - fieldStart;
            byte[] result = new byte[length];
            System.arraycopy(certificate, fieldStart, result, 0, length);
            return result;
          }
          fieldCount++;
          fieldStart = -1;
        }

        // Skip whitespace
        while (i < certificate.length && isWhitespace(certificate[i])) {
          i++;
        }
      }
    }
    // Handle last field (no trailing whitespace)
    if (fieldStart != -1 && fieldCount == index) {
      int length = i - fieldStart;
      byte[] result = new byte[length];
      System.arraycopy(certificate, fieldStart, result, 0, length);
      return result;
    }

    return null;
  }

  /**
   * Determines whether the given {@link OpenSshCertificate} is valid at the current local system
   * time.
   *
   * @param cert to check
   * @return {@code true} if the certificate is valid according to its timestamps, {@code false}
   *         otherwise
   */
  static boolean isValidNow(OpenSshCertificate cert) {
    return isValidNow(cert, Instant.now().getEpochSecond());
  }

  /**
   * Determines whether the given {@link OpenSshCertificate} is valid at the given time.
   *
   * @param cert to check
   * @param now the current time in seconds since epoch
   * @return {@code true} if the certificate is valid according to its timestamps, {@code false}
   *         otherwise
   */
  static boolean isValidNow(OpenSshCertificate cert, long now) {
    return Long.compareUnsigned(cert.getValidAfter(), now) <= 0
        && Long.compareUnsigned(now, cert.getValidBefore()) < 0;
  }

  /**
   * Converts a Unix timestamp to a {@link Date} string representation.
   * <p>
   * If the timestamp is negative, it indicates an infinite expiration time, and the method returns
   * the string "infinity". Otherwise, it converts the timestamp from seconds to milliseconds and
   * returns the default string representation of the resulting {@link Date} object.
   *
   * @param timestamp The Unix timestamp in seconds.
   * @return A string representing the date, or "infinity" if the timestamp is negative.
   */
  static String toDateString(long timestamp) {
    if (timestamp < 0) {
      return "infinity";
    }
    return SftpATTRS.toDateString(timestamp);
  }

  /**
   * Extracts the raw key type from a given key type string.
   * <p>
   * For certificate key types (ending with {@code -cert-v01@openssh.com}), this method returns the
   * base algorithm name (e.g., {@code ssh-rsa-cert-v01@openssh.com} returns {@code ssh-rsa}). For
   * non-certificate key types, the original string is returned unchanged.
   * </p>
   * <p>
   * This method delegates to {@link OpenSshCertificateKeyTypes#getBaseKeyType(String)}.
   * </p>
   *
   * @param keyType The full key type string, may be null.
   * @return The raw key type (e.g., "ssh-rsa"), or the original string if not a certificate type,
   *         or null if the input is null or empty.
   */
  static String getRawKeyType(String keyType) {
    return OpenSshCertificateKeyTypes.getBaseKeyType(keyType);
  }

  /**
   * Filters out OpenSSH certificate types whose underlying signature algorithms are unavailable.
   * <p>
   * This method ensures that JSch does not attempt to negotiate certificate-based host key
   * algorithms when the corresponding signature implementation is unavailable on the system. This
   * prevents connection failures that would occur if JSch negotiated a certificate type (e.g.,
   * {@code ssh-ed25519-cert-v01@openssh.com}) but could not verify it because the base signature
   * algorithm (e.g., {@code ssh-ed25519}) requires Java 15+ or Bouncy Castle.
   * </p>
   *
   * <h3>Algorithm Mapping</h3>
   * <p>
   * The method maintains an internal mapping between base signature algorithms and their
   * certificate counterparts:
   * </p>
   * <ul>
   * <li>{@code ssh-ed25519} → {@code ssh-ed25519-cert-v01@openssh.com}</li>
   * <li>{@code ssh-ed448} → {@code ssh-ed448-cert-v01@openssh.com}</li>
   * <li>{@code ssh-rsa} → {@code ssh-rsa-cert-v01@openssh.com}</li>
   * <li>{@code rsa-sha2-256} → {@code rsa-sha2-256-cert-v01@openssh.com}</li>
   * <li>{@code rsa-sha2-512} → {@code rsa-sha2-512-cert-v01@openssh.com}</li>
   * <li>{@code ssh-dss} → {@code ssh-dss-cert-v01@openssh.com}</li>
   * <li>{@code ecdsa-sha2-nistp256} → {@code ecdsa-sha2-nistp256-cert-v01@openssh.com}</li>
   * <li>{@code ecdsa-sha2-nistp384} → {@code ecdsa-sha2-nistp384-cert-v01@openssh.com}</li>
   * <li>{@code ecdsa-sha2-nistp521} → {@code ecdsa-sha2-nistp521-cert-v01@openssh.com}</li>
   * </ul>
   *
   * @param serverHostKey comma-separated list of server host key algorithms to filter. This
   *        typically contains a mix of plain key algorithms (e.g., {@code ssh-ed25519}) and
   *        certificate types (e.g., {@code ssh-ed25519-cert-v01@openssh.com}). May be {@code null}.
   * @param unavailableSignatures array of base signature algorithms that are unavailable on this
   *        system, as determined by {@link Session#checkSignatures(String)}. Each entry is a plain
   *        algorithm name like {@code ssh-ed25519} or {@code rsa-sha2-512}. May be {@code null} or
   *        empty if all signature algorithms are available.
   * @return the filtered comma-separated list of server host key algorithms with unavailable
   *         certificate types removed, or {@code null} if all algorithms were filtered out. If
   *         {@code unavailableSignatures} is {@code null} or empty, returns {@code serverHostKey}
   *         unchanged.
   */
  static String filterUnavailableCertTypes(String serverHostKey, String[] unavailableSignatures) {
    if (unavailableSignatures == null || unavailableSignatures.length == 0) {
      return serverHostKey;
    }

    if (JSch.getLogger().isEnabled(Logger.DEBUG)) {
      JSch.getLogger().log(Logger.DEBUG,
          "server_host_key proposal before removing unavailable cert types is: " + serverHostKey);
    }

    // Build list of certificate types to remove based on unavailable base signatures
    List<String> certsToRemove = new ArrayList<String>();

    for (String unavailableSig : unavailableSignatures) {
      // Map base algorithm to corresponding certificate type using centralized mapping
      String certType = OpenSshCertificateKeyTypes.getCertificateKeyType(unavailableSig);
      if (certType != null) {
        certsToRemove.add(certType);
      }
    }

    if (!certsToRemove.isEmpty()) {
      String[] certsArray = new String[certsToRemove.size()];
      certsToRemove.toArray(certsArray);
      serverHostKey = Util.diffString(serverHostKey, certsArray);

      if (JSch.getLogger().isEnabled(Logger.DEBUG)) {
        for (String cert : certsArray) {
          JSch.getLogger().log(Logger.DEBUG, "Removing " + cert + " (base algorithm unavailable)");
        }
        JSch.getLogger().log(Logger.DEBUG,
            "server_host_key proposal after removing unavailable cert types is: " + serverHostKey);
      }
    }
    return serverHostKey;
  }

  /**
   * Validates that a certificate is signed by a trusted, non-revoked Certificate Authority.
   * <p>
   * This method performs the critical CA validation step for OpenSSH certificate authentication. It
   * verifies that:
   * </p>
   * <ol>
   * <li>The CA public key exists in the known_hosts file with {@code @cert-authority} marker</li>
   * <li>The CA entry matches the connecting host's pattern</li>
   * <li>The CA key has not been revoked (no {@code @revoked} entry for same key)</li>
   * <li>The certificate was signed by this CA (CA public key matches)</li>
   * </ol>
   *
   * <h3>Validation Flow</h3>
   * <p>
   * The validation follows these steps:
   * </p>
   *
   * <pre>
   * 1. Retrieve all {@code @cert-authority} entries from known_hosts
   * 2. Filter to only non-null entries
   * 3. Check each CA to ensure it hasn't been revoked
   * 4. Test if any remaining CA:
   *    - Matches the host pattern (e.g., *.example.com matches host.example.com)
   *    - Has a public key that equals the certificate's signing CA key
   * </pre>
   *
   * <h3>Revocation Checking</h3>
   * <p>
   * A CA is considered revoked if there exists a {@code @revoked} entry in the known_hosts file
   * with the same public key value. If a matching CA for the given host and key is found to be
   * revoked, a {@link JSchRevokedHostKeyException} is thrown immediately. This is a hard failure
   * that must not be caught for fallback purposes, matching OpenSSH behavior where {@code @revoked}
   * is a terminal rejection.
   * </p>
   *
   *
   * @param repository the {@link HostKeyRepository} containing known_hosts entries, must not be
   *        {@code null}
   * @param host the hostname or host pattern being connected to (e.g., "host.example.com" or
   *        "[host.example.com]:2222"), must not be {@code null}
   * @param caPublicKey the raw CA public key bytes from the certificate that needs validation, must
   *        not be {@code null}
   * @return {@code true} if a trusted, non-revoked CA matching the host pattern signed the
   *         certificate; {@code false} if no matching CA exists or the CA key doesn't match
   * @throws JSchRevokedHostKeyException if the matching CA key has been revoked
   */
  static boolean isCertificateSignedByTrustedCA(HostKeyRepository repository, String host,
      byte[] caPublicKey) throws JSchException {
    // First, check if any matching CA has been revoked — revocation is a hard failure
    // that must not fall back to plain key checking (matches OpenSSH behavior)
    boolean matchingCARevoked = getTrustedCAs(repository).stream().filter(Objects::nonNull)
        .filter(hostkey -> hasBeenRevoked(repository, hostkey)).anyMatch(revokedCA -> {
          byte[] revokedCAKeyBytes = revokedCA.key;
          if (revokedCAKeyBytes == null) {
            return false;
          }
          return revokedCA.isWildcardMatched(host)
              && Util.arraysequals(revokedCAKeyBytes, caPublicKey);
        });
    if (matchingCARevoked) {
      throw new JSchRevokedHostKeyException(
          "Rejected certificate: signing CA key has been revoked for " + host);
    }

    return getTrustedCAs(repository).stream().filter(Objects::nonNull)
        .filter(hostkey -> !hasBeenRevoked(repository, hostkey)).anyMatch(trustedCA -> {
          byte[] trustedCAKeyBytes = trustedCA.key;
          if (trustedCAKeyBytes == null) {
            return false;
          }
          return trustedCA.isWildcardMatched(host)
              && Util.arraysequals(trustedCAKeyBytes, caPublicKey);
        });
  }

  /**
   * Retrieves all trusted Certificate Authority (CA) host keys from the repository.
   * <p>
   * This method extracts all entries from the known_hosts file that are marked with the
   * {@code @cert-authority} marker, which designates them as trusted CAs for certificate-based host
   * authentication.
   * </p>
   *
   * @param knownHosts the {@link HostKeyRepository} to query (typically populated from a
   *        known_hosts file), may be empty but must not be {@code null}
   * @return a {@link Set} of {@link HostKey} objects representing all CA entries; returns empty set
   *         if repository is empty or contains no CA entries, never returns {@code null}
   */
  static Set<HostKey> getTrustedCAs(HostKeyRepository knownHosts) {
    HostKey[] hostKeys = knownHosts.getHostKey();
    return hostKeys == null ? new HashSet<>()
        : Arrays.stream(hostKeys).filter(OpenSshCertificateUtil::isKnownHostCaPublicKeyEntry)
            .collect(Collectors.toSet());
  }

  /**
   * Retrieves all revoked key entries from the repository.
   * <p>
   * This method extracts all entries from the known_hosts file that are marked with the
   * {@code @revoked} marker, indicating keys that have been explicitly blacklisted and must not be
   * trusted for authentication.
   * </p>
   * <p>
   * Revoked entries take precedence over trusted entries. If a key appears in both:
   * </p>
   * <ul>
   * <li>A {@code @cert-authority} or regular trusted entry, AND</li>
   * <li>A {@code @revoked} entry</li>
   * </ul>
   * <p>
   * The key must be rejected. Use {@link #hasBeenRevoked(HostKeyRepository, HostKey)} to check if a
   * specific key has been revoked.
   * </p>
   *
   * @param knownHosts the {@link HostKeyRepository} to query (typically populated from a
   *        known_hosts file), may be empty but must not be {@code null}
   * @return a {@link Set} of {@link HostKey} objects representing all revoked entries (includes
   *         {@code null} entries due to fail-closed security); returns empty set if repository
   *         contains no revoked entries, never returns {@code null}
   */
  static Set<HostKey> getRevokedKeys(HostKeyRepository knownHosts) {
    HostKey[] hostKeys = knownHosts.getHostKey();
    return hostKeys == null ? new HashSet<>()
        : Arrays.stream(hostKeys).filter(OpenSshCertificateUtil::isMarkedRevoked)
            .collect(Collectors.toSet());
  }

  /**
   * Checks if a certificate's embedded public key has been revoked.
   * <p>
   * This method checks whether the public key contained within a host certificate appears in any
   * {@code @revoked} entry in the known_hosts file. This is separate from CA revocation — even if
   * the CA is trusted, the individual host key may have been compromised and explicitly revoked.
   * </p>
   * <p>
   * This matches OpenSSH behavior in {@code check_key_not_revoked()} (hostfile.c), which checks
   * both the host key itself and the CA signing key against {@code @revoked} entries.
   * </p>
   *
   * @param knownHosts the {@link HostKeyRepository} to query for revoked entries, must not be
   *        {@code null}
   * @param certPublicKey the raw public key bytes embedded in the certificate, must not be
   *        {@code null}
   * @return {@code true} if the certificate's public key matches any {@code @revoked} entry;
   *         {@code false} otherwise
   */
  static boolean isCertificateKeyRevoked(HostKeyRepository knownHosts, byte[] certPublicKey) {
    if (certPublicKey == null) {
      return true;
    }
    return getRevokedKeys(knownHosts).stream().filter(Objects::nonNull)
        .map(revokedKey -> revokedKey.key).filter(Objects::nonNull)
        .anyMatch(revokedKeyBytes -> Util.arraysequals(revokedKeyBytes, certPublicKey));
  }

  /**
   * Checks if a given host key has been revoked.
   * <p>
   * This method determines whether a {@link HostKey} appears in the known_hosts file with the
   * {@code @revoked} marker, indicating it should not be trusted for authentication. It compares
   * the key's public key value against all revoked entries.
   * </p>
   *
   * @param knownHosts the {@link HostKeyRepository} to query for revoked entries, must not be
   *        {@code null}
   * @param key the {@link HostKey} to check for revocation, may be {@code null}
   * @return {@code true} if {@code key} is {@code null} (fail-closed) or if the key's public key
   *         value matches any {@code @revoked} entry in the repository; {@code false} if the key is
   *         valid and not revoked
   */
  static boolean hasBeenRevoked(HostKeyRepository knownHosts, HostKey key) {
    if (key == null) {
      return true;
    }
    byte[] keyBytes = key.key;
    if (keyBytes == null) {
      // Fail-closed: treat keys with null key value as revoked
      return true;
    }
    // Compare raw byte arrays directly using timing-safe comparison
    return getRevokedKeys(knownHosts).stream().filter(Objects::nonNull)
        .map(revokedKey -> revokedKey.key).filter(Objects::nonNull)
        .anyMatch(revokedKeyBytes -> Util.arraysequals(revokedKeyBytes, keyBytes));
  }

  /**
   * Parses a raw public key byte array into its constituent mathematical components.
   * <p>
   * Different public key algorithms (RSA, DSS, ECDSA, EdDSA) have different structures. This method
   * decodes the algorithm-specific format and returns the components needed for cryptographic
   * operations.
   * </p>
   *
   * @param publicKeyBlob the raw byte array of the public key blob in SSH wire format.
   * @return A 2D byte array where each inner array is a component of the public key.
   * @throws JSchException if the public key algorithm is unknown or the key format is corrupt.
   */
  static byte[][] parsePublicKeyComponents(byte[] publicKeyBlob) throws JSchException {
    Buffer buffer = new Buffer(publicKeyBlob);
    String algorithm = Util.byte2str(buffer.getString());
    return parsePublicKeyComponentsFromBuffer(algorithm, buffer);
  }

  /**
   * Parses public key components from a buffer based on the algorithm.
   * <p>
   * This method reads key components from the current buffer position. The buffer should be
   * positioned after the algorithm string.
   * </p>
   *
   * @param algorithm the public key algorithm name.
   * @param buffer the buffer containing the key components.
   * @return A 2D byte array where each inner array is a component of the public key.
   * @throws JSchException if the algorithm is unknown or the key format is invalid.
   */
  static byte[][] parsePublicKeyComponentsFromBuffer(String algorithm, Buffer buffer)
      throws JSchException {

    if (algorithm.startsWith("ssh-rsa") || algorithm.startsWith("rsa-")) {
      byte[] e = buffer.getMPInt();
      byte[] n = buffer.getMPInt();
      return new byte[][] {e, n};
    }

    if (algorithm.startsWith("ssh-dss")) {
      byte[] p = buffer.getMPInt();
      byte[] q = buffer.getMPInt();
      byte[] g = buffer.getMPInt();
      byte[] y = buffer.getMPInt();
      // Order matches SignatureDSA.setPubKey(y, p, q, g)
      return new byte[][] {y, p, q, g};
    }

    if (algorithm.startsWith("ecdsa-sha2-")) {
      // https://www.rfc-editor.org/rfc/rfc5656#section-3.1
      buffer.getString(); // skip curve identifier
      int len = buffer.getInt();
      if (len < ECDSA_P256_POINT_LENGTH || len > ECDSA_P521_POINT_LENGTH) {
        throw new JSchException("Invalid ECDSA public key length: " + len + " (expected between "
            + ECDSA_P256_POINT_LENGTH + " and " + ECDSA_P521_POINT_LENGTH + ")");
      }
      int pointFormat = buffer.getByte();
      if (pointFormat != EC_POINT_FORMAT_UNCOMPRESSED) {
        throw new JSchException(
            "Invalid ECDSA public key format: expected uncompressed point (0x04), got 0x"
                + Integer.toHexString(pointFormat & 0xff));
      }
      byte[] r = new byte[(len - 1) / 2];
      byte[] s = new byte[(len - 1) / 2];
      buffer.getByte(r);
      buffer.getByte(s);
      // Order matches SignatureECDSA.setPubKey(r, s)
      return new byte[][] {r, s};
    }

    if (algorithm.startsWith("ssh-ed25519")) {
      int keyLength = buffer.getInt();
      if (keyLength != ED25519_PUBLIC_KEY_LENGTH) {
        throw new JSchException("Invalid Ed25519 public key length: expected "
            + ED25519_PUBLIC_KEY_LENGTH + ", got " + keyLength);
      }
      byte[] pubKey = new byte[keyLength];
      buffer.getByte(pubKey);
      return new byte[][] {pubKey};
    }

    if (algorithm.startsWith("ssh-ed448")) {
      int keyLength = buffer.getInt();
      if (keyLength != ED448_PUBLIC_KEY_LENGTH) {
        throw new JSchException("Invalid Ed448 public key length: expected "
            + ED448_PUBLIC_KEY_LENGTH + ", got " + keyLength);
      }
      byte[] pubKey = new byte[keyLength];
      buffer.getByte(pubKey);
      return new byte[][] {pubKey};
    }

    throw new JSchUnknownPublicKeyAlgorithmException(
        "Unknown algorithm '" + algorithm.trim() + "'");
  }
}
