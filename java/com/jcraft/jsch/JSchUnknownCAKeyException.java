package com.jcraft.jsch;

public class JSchUnknownCAKeyException extends JSchHostKeyException {

  private static final long serialVersionUID = -1L;

  JSchUnknownCAKeyException(String s) {
    super(s);
  }
}
