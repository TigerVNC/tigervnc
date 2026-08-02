package com.jcraft.jsch;

public class JplLogger implements com.jcraft.jsch.Logger {

  public JplLogger() {
    throw new UnsupportedOperationException("JplLogger requires Java9+.");
  }

  @Override
  public boolean isEnabled(int level) {
    throw new UnsupportedOperationException("JplLogger requires Java9+.");
  }

  @Override
  public void log(int level, String message) {
    throw new UnsupportedOperationException("JplLogger requires Java9+.");
  }

  @Override
  public void log(int level, String message, Throwable cause) {
    throw new UnsupportedOperationException("JplLogger requires Java9+.");
  }
}
