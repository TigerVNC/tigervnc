package com.jcraft.jsch;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * A specialized buffer for parsing OpenSSH certificate data.
 *
 * <p>
 * This class extends the base {@link Buffer} class to provide additional methods specific to
 * parsing OpenSSH certificate format data structures, including string collections, key-value maps,
 * and certificate-specific data types.
 * </p>
 *
 * <p>
 * The buffer follows the SSH wire format protocol for data serialization, where strings and byte
 * arrays are prefixed with their length as a 32-bit integer.
 * </p>
 */
class OpenSshCertificateBuffer extends Buffer {

  /**
   * Creates a new OpenSSH certificate buffer from decoded certificate bytes.
   *
   * @param certificateByteDecoded the decoded certificate data
   */
  OpenSshCertificateBuffer(byte[] certificateByteDecoded) {
    super(certificateByteDecoded);
    s = 0;
    index = certificateByteDecoded.length;
  }

  /**
   * Reads a length-prefixed byte array from the buffer.
   *
   * @return the byte array data
   * @throws IllegalArgumentException if the length prefix is negative or exceeds available data
   */
  byte[] getBytes() {
    int reqLen = getInt();
    if (reqLen < 0) {
      throw new IllegalArgumentException(
          "Invalid length in certificate data: negative length " + reqLen);
    }
    if (reqLen > getLength()) {
      throw new IllegalArgumentException("Invalid length in certificate data: requested " + reqLen
          + " bytes but only " + getLength() + " available");
    }
    byte[] b = new byte[reqLen];
    getByte(b);
    return b;
  }

  /**
   * Reads a collection of UTF-8 encoded strings from the buffer.
   *
   * <p>
   * This method reads all remaining data in the buffer and parses it as a sequence of
   * length-prefixed UTF-8 strings.
   * </p>
   *
   * @return collection of strings
   */
  Collection<String> getStrings() {
    List<String> list = new ArrayList<>();
    while (getLength() > 0) {
      String s = Util.byte2str(getString(), StandardCharsets.UTF_8);
      list.add(s);
    }
    return list;
  }

  /**
   * Reads critical options from the buffer.
   *
   * <p>
   * Critical options are stored as key-value pairs in SSH wire format.
   * </p>
   *
   * @return map of critical option names to values
   */
  Map<String, String> getCriticalOptions() {
    return getKeyValueData();
  }

  /**
   * Reads extensions from the buffer.
   *
   * <p>
   * Extensions are stored as key-value pairs in SSH wire format.
   * </p>
   *
   * @return map of extension names to values
   */
  Map<String, String> getExtensions() {
    return getKeyValueData();
  }

  /**
   * Reads key-value pair data from the buffer.
   *
   * <p>
   * This method handles the SSH wire format for storing maps, where the entire map is first stored
   * as a length-prefixed blob, followed by alternating keys and values, each also length-prefixed.
   * </p>
   *
   * @return map of keys to values
   */
  private Map<String, String> getKeyValueData() {
    Map<String, String> map = new LinkedHashMap<>();

    if (getLength() > 0) {
      OpenSshCertificateBuffer keyValueDataBuffer = new OpenSshCertificateBuffer(getString());
      while (keyValueDataBuffer.getLength() > 0) {
        String key = Util.byte2str(keyValueDataBuffer.getString(), StandardCharsets.UTF_8);
        String value = Util.byte2str(keyValueDataBuffer.getString(), StandardCharsets.UTF_8);
        map.put(key, value);
      }
    }
    return map;
  }
}
