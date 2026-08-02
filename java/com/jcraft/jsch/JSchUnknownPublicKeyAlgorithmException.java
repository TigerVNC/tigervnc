package com.jcraft.jsch;

public class JSchUnknownPublicKeyAlgorithmException extends JSchHostKeyException {

  private static final long serialVersionUID = -1L;

  JSchUnknownPublicKeyAlgorithmException(String s) {
    super(s);
  }
}
