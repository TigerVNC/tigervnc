package com.jcraft.jsch;

public class JSchInvalidHostCertificateException extends JSchHostKeyException {

  private static final long serialVersionUID = -1L;

  JSchInvalidHostCertificateException(String s) {
    super(s);
  }
}
