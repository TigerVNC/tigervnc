/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2013-2015 ymnk, JCraft,Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in 
     the documentation and/or other materials provided with the distribution.

  3. The names of the authors may not be used to endorse or promote products
     derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JCRAFT,
INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package com.jcraft.jsch;

import java.io.InputStream;
import java.io.Reader;
import java.io.StringReader;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.util.Hashtable;
import java.util.Vector;

/**
 * This class implements ConfigRepository interface, and parses
 * OpenSSH's configuration file.  The following keywords will be recognized,
 * <ul>
 *   <li>Host</li>
 *   <li>User</li>
 *   <li>Hostname</li>
 *   <li>Port</li>
 *   <li>PreferredAuthentications</li>
 *   <li>IdentityFile</li>
 *   <li>NumberOfPasswordPrompts</li>
 *   <li>ConnectTimeout</li>
 *   <li>HostKeyAlias</li>
 *   <li>UserKnownHostsFile</li>
 *   <li>KexAlgorithms</li>
 *   <li>HostKeyAlgorithms</li>
 *   <li>Ciphers</li>
 *   <li>Macs</li>
 *   <li>Compression</li>
 *   <li>CompressionLevel</li>
 *   <li>ForwardAgent</li>
 *   <li>RequestTTY</li>
 *   <li>ServerAliveInterval</li>  
 *   <li>LocalForward</li>
 *   <li>RemoteForward</li>
 *   <li>ClearAllForwardings</li>
 * </ul>
 *
 * @see ConfigRepository
 */
public class OpenSSHConfig implements ConfigRepository {

  /**
   * Parses the given string, and returns an instance of ConfigRepository.
   *
   * @param conf string, which includes OpenSSH's config
   * @return an instanceof OpenSSHConfig
   */
  public static OpenSSHConfig parse(String conf) throws IOException {
    Reader r = new StringReader(conf);
    try {
      return new OpenSSHConfig(r);
    }
    finally {
      r.close();
    }
  }

  /**
   * Parses the given file, and returns an instance of ConfigRepository.
   *
   * @param file OpenSSH's config file
   * @return an instanceof OpenSSHConfig
   */
  public static OpenSSHConfig parseFile(String file) throws IOException {
    Reader r = new FileReader(Util.checkTilde(file));
    try {
      return new OpenSSHConfig(r);
    }
    finally {
      r.close();
    }
  }

  OpenSSHConfig(Reader r) throws IOException {
    _parse(r);
  }

  private final Hashtable config = new Hashtable();
  private final Vector hosts = new Vector();

  private void _parse(Reader r) throws IOException {
    BufferedReader br = new BufferedReader(r);

    String host = "";
    Vector/*<String[]>*/ kv = new Vector();
    String l = null;

    while((l = br.readLine()) != null){
      l = l.trim();
      if(l.length() == 0 || l.startsWith("#"))
        continue;

      String[] key_value = l.split("[= \t]", 2);
      for(int i = 0; i < key_value.length; i++)
        key_value[i] = key_value[i].trim();

      if(key_value.length <= 1)
        continue;

      if(key_value[0].equals("Host")){
        config.put(host, kv);
        hosts.addElement(host);
        host = key_value[1];
        kv = new Vector();
      }
      else {
        kv.addElement(key_value);
      }
    }
    config.put(host, kv);
    hosts.addElement(host);
  }

  public Config getConfig(String host) {
    return new MyConfig(host);
  }

  private static final Hashtable keymap = new Hashtable();
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
  }

  class MyConfig implements Config {

    private String host;
    private Vector _configs = new Vector();

    MyConfig(String host){
      this.host = host;

      _configs.addElement(config.get(""));

      byte[] _host = Util.str2byte(host);
      if(hosts.size() > 1){
        for(int i = 1; i < hosts.size(); i++){
          String patterns[] = ((String)hosts.elementAt(i)).split("[ \t]");
          for(int j = 0; j < patterns.length; j++){
            boolean negate = false;
            String foo = patterns[j].trim();
            if(foo.startsWith("!")){
              negate = true;
              foo = foo.substring(1).trim();
            }
            if(Util.glob(Util.str2byte(foo), _host)){
              if(!negate){
                _configs.addElement(config.get((String)hosts.elementAt(i)));
              }
            }
            else if(negate){
              _configs.addElement(config.get((String)hosts.elementAt(i)));
            }
          }
        }
      }
    }

    private String find(String key) {
      if(keymap.get(key)!=null) {
        key = (String)keymap.get(key);
      }
      key = key.toUpperCase();
      String value = null;
      for(int i = 0; i < _configs.size(); i++) {
        Vector v = (Vector)_configs.elementAt(i);
        for(int j = 0; j < v.size(); j++) {
          String[] kv = (String[])v.elementAt(j);
          if(kv[0].toUpperCase().equals(key)) {
            value = kv[1];
            break;
          }
        }
        if(value != null)
          break;
      }
      return value;
    }

    private String[] multiFind(String key) {
      key = key.toUpperCase();
      Vector value = new Vector();
      for(int i = 0; i < _configs.size(); i++) {
        Vector v = (Vector)_configs.elementAt(i);
        for(int j = 0; j < v.size(); j++) {
          String[] kv = (String[])v.elementAt(j);
          if(kv[0].toUpperCase().equals(key)) {
            String foo = kv[1];
            if(foo != null) {
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

    public String getHostname(){ return find("Hostname"); }
    public String getUser(){ return find("User"); }
    public int getPort(){
      String foo = find("Port");
      int port = -1;
      try {
        port = Integer.parseInt(foo);
      }
      catch(NumberFormatException e){
        // wrong format
      }
      return port;
    }
    public String getValue(String key){
      if(key.equals("compression.s2c") ||
         key.equals("compression.c2s")) {
        String foo = find(key);
        if(foo == null || foo.equals("no"))
          return "none,zlib@openssh.com,zlib";
        return "zlib@openssh.com,zlib,none";
      }
      return find(key);
    }
    public String[] getValues(String key){ return multiFind(key); }
  }
}
