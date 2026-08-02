/*
 * Copyright (c) 2013-2018 ymnk, JCraft,Inc. All rights reserved.
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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.Hashtable;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.Vector;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * This class implements ConfigRepository interface, and parses OpenSSH's configuration file. The
 * following keywords will be recognized,
 *
 * <ul>
 * <li>Host</li>
 * <li>User</li>
 * <li>Hostname</li>
 * <li>Port</li>
 * <li>PreferredAuthentications</li>
 * <li>PubkeyAcceptedAlgorithms</li>
 * <li>FingerprintHash</li>
 * <li>IdentityFile</li>
 * <li>NumberOfPasswordPrompts</li>
 * <li>ConnectTimeout</li>
 * <li>HostKeyAlias</li>
 * <li>UserKnownHostsFile</li>
 * <li>KexAlgorithms</li>
 * <li>HostKeyAlgorithms</li>
 * <li>Ciphers</li>
 * <li>Macs</li>
 * <li>Compression</li>
 * <li>CompressionLevel</li>
 * <li>ForwardAgent</li>
 * <li>RequestTTY</li>
 * <li>ServerAliveInterval</li>
 * <li>LocalForward</li>
 * <li>RemoteForward</li>
 * <li>ClearAllForwardings</li>
 * <li>CASignatureAlgorithms</li>
 * </ul>
 *
 * @see ConfigRepository
 */
public class OpenSSHConfig implements ConfigRepository {

  private static final Set<String> keysWithListAdoption = Stream
      .of("KexAlgorithms", "Ciphers", "HostKeyAlgorithms", "MACs", "PubkeyAcceptedAlgorithms",
          "PubkeyAcceptedKeyTypes", "CASignatureAlgorithms")
      .map(string -> string.toUpperCase(Locale.ROOT)).collect(Collectors.toSet());

  /**
   * Parses the given string, and returns an instance of ConfigRepository.
   *
   * @param conf string, which includes OpenSSH's config
   * @return an instanceof OpenSSHConfig
   */
  public static OpenSSHConfig parse(String conf) throws IOException {
    try (Reader r = new StringReader(conf)) {
      try (BufferedReader br = new BufferedReader(r)) {
        return new OpenSSHConfig(br);
      }
    }
  }

  /**
   * Parses the given file, and returns an instance of ConfigRepository.
   *
   * @param file OpenSSH's config file
   * @return an instanceof OpenSSHConfig
   */
  public static OpenSSHConfig parseFile(String file) throws IOException {
    try (BufferedReader br =
        Files.newBufferedReader(Paths.get(Util.checkTilde(file)), StandardCharsets.UTF_8)) {
      return new OpenSSHConfig(br);
    }
  }

  OpenSSHConfig(BufferedReader br) throws IOException {
    _parse(br);
  }

  private final Hashtable<String, Vector<String[]>> config = new Hashtable<>();
  private final Vector<String> hosts = new Vector<>();

  private void _parse(BufferedReader br) throws IOException {
    String host = "";
    Vector<String[]> kv = new Vector<>();
    String l = null;

    while ((l = br.readLine()) != null) {
      l = l.trim();
      if (l.length() == 0 || l.startsWith("#"))
        continue;

      String[] key_value = l.split("[= \t]", 2);
      for (int i = 0; i < key_value.length; i++)
        key_value[i] = key_value[i].trim();

      if (key_value.length <= 1)
        continue;

      if (key_value[0].equalsIgnoreCase("Host")) {
        config.put(host, kv);
        hosts.addElement(host);
        host = key_value[1];
        kv = new Vector<>();
      } else {
        kv.addElement(key_value);
      }
    }
    config.put(host, kv);
    hosts.addElement(host);
  }

  @Override
  public Config getConfig(String host) {
    return new MyConfig(host);
  }

  /**
   * Returns mapping of jsch config property names to OpenSSH property names.
   *
   * @return map
   */
  static Hashtable<String, String> getKeymap() {
    return keymap;
  }

  private static final Hashtable<String, String> keymap = new Hashtable<>();

  static {
    keymap.put("kex", "KexAlgorithms");
    keymap.put("server_host_key", "HostKeyAlgorithms");
    keymap.put("cipher.c2s", "Ciphers");
    keymap.put("cipher.s2c", "Ciphers");
    keymap.put("mac.c2s", "Macs");
    keymap.put("mac.s2c", "Macs");
    keymap.put("compression.s2c", "Compression");
    keymap.put("compression.c2s", "Compression");
    keymap.put("compression_level", "CompressionLevel");
    keymap.put("MaxAuthTries", "NumberOfPasswordPrompts");
    keymap.put("ca_signature_algorithms", "CASignatureAlgorithms");
  }

  class MyConfig implements Config {

    private String host;
    private Vector<Vector<String[]>> _configs = new Vector<>();

    MyConfig(String host) {
      this.host = host;

      _configs.addElement(config.get(""));

      byte[] _host = Util.str2byte(host);
      if (hosts.size() > 1) {
        for (int i = 1; i < hosts.size(); i++) {
          boolean anyPositivePatternMatches = false;
          boolean anyNegativePatternMatches = false;
          String patterns[] = hosts.elementAt(i).split("[ \t]");
          for (int j = 0; j < patterns.length; j++) {
            boolean negate = false;
            String foo = patterns[j].trim();
            if (foo.startsWith("!")) {
              negate = true;
              foo = foo.substring(1).trim();
            }
            if (Util.glob(Util.str2byte(foo), _host)) {
              if (negate) {
                anyNegativePatternMatches = true;
              } else {
                anyPositivePatternMatches = true;
              }
            }
          }

          if (anyPositivePatternMatches && !anyNegativePatternMatches) {
            _configs.addElement(config.get(hosts.elementAt(i)));
          }
        }
      }
    }

    private String find(String key) {
      String originalKey = key;
      if (keymap.get(key) != null) {
        key = keymap.get(key);
      }
      key = key.toUpperCase(Locale.ROOT);
      String value = null;
      for (int i = 0; i < _configs.size(); i++) {
        Vector<String[]> v = _configs.elementAt(i);
        for (int j = 0; j < v.size(); j++) {
          String[] kv = v.elementAt(j);
          if (kv[0].toUpperCase(Locale.ROOT).equals(key)) {
            value = kv[1];
            break;
          }
        }
        if (value != null)
          break;
      }

      if (value != null && (key.equals("SERVERALIVEINTERVAL") || key.equals("CONNECTTIMEOUT"))) {
        try {
          int timeout = Integer.parseInt(value);
          value = Integer.toString(timeout * 1000);
        } catch (NumberFormatException e) {
          logError(originalKey, e);
        }
      }

      if (keysWithListAdoption.contains(key) && value != null
          && (value.startsWith("+") || value.startsWith("-") || value.startsWith("^"))) {

        String origConfig = JSch.getConfig(originalKey).trim();

        if (value.startsWith("+")) {
          value = origConfig + "," + value.substring(1).trim();
        } else if (value.startsWith("-")) {
          List<String> algList =
              Arrays.stream(Util.split(origConfig, ",")).collect(Collectors.toList());
          for (String alg : Util.split(value.substring(1).trim(), ",")) {
            algList.remove(alg.trim());
          }
          value = String.join(",", algList);
        } else if (value.startsWith("^")) {
          value = value.substring(1).trim() + "," + origConfig;
        }
      }

      return value;
    }

    private void logError(String originalKey, NumberFormatException e) {
      Logger logger = JSch.getLogger();
      if (logger != null) {
        logger.log(Logger.ERROR, "Error during parsing of " + originalKey + ": " + e.getMessage(),
            e);
      }
    }

    private String[] multiFind(String key) {
      key = key.toUpperCase(Locale.ROOT);
      Vector<String> value = new Vector<>();
      for (int i = 0; i < _configs.size(); i++) {
        Vector<String[]> v = _configs.elementAt(i);
        for (int j = 0; j < v.size(); j++) {
          String[] kv = v.elementAt(j);
          if (kv[0].toUpperCase(Locale.ROOT).equals(key)) {
            String foo = kv[1];
            if (foo != null) {
              value.remove(foo);
              value.addElement(foo);
            }
          }
        }
      }
      String[] result = new String[value.size()];
      value.toArray(result);
      return result;
    }

    @Override
    public String getHostname() {
      return find("Hostname");
    }

    @Override
    public String getUser() {
      return find("User");
    }

    @Override
    public int getPort() {
      String foo = find("Port");
      int port = -1;
      // Port is not required and we don't want to log a failure if its simply missing from the
      // OpenSSH config
      if (foo != null) {
        try {
          port = Integer.parseInt(foo);
        } catch (NumberFormatException e) {
          logError("Port", e);
        }
      }
      return port;
    }

    @Override
    public String getValue(String key) {
      if (key.equals("compression.s2c") || key.equals("compression.c2s")) {
        String foo = find(key);
        if (foo == null || foo.equals("no"))
          return "none,zlib@openssh.com,zlib";
        return "zlib@openssh.com,zlib,none";
      }
      return find(key);
    }

    @Override
    public String[] getValues(String key) {
      return multiFind(key);
    }
  }
}
