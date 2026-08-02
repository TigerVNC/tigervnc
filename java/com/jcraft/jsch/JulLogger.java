package com.jcraft.jsch;

import java.util.logging.Level;
import java.util.logging.Logger;

public class JulLogger implements com.jcraft.jsch.Logger {

  private static final Logger logger = Logger.getLogger(JSch.class.getName());

  public JulLogger() {}

  @Override
  public boolean isEnabled(int level) {
    return logger.isLoggable(getLevel(level));
  }

  @Override
  public void log(int level, String message) {
    log(level, message, null);
  }

  @Override
  public void log(int level, String message, Throwable cause) {
    if (cause == null) {
      logger.log(getLevel(level), message);
      return;
    }
    logger.log(getLevel(level), message, cause);
  }

  static Level getLevel(int level) {
    switch (level) {
      case com.jcraft.jsch.Logger.DEBUG:
        return Level.FINE;
      case com.jcraft.jsch.Logger.INFO:
        return Level.INFO;
      case com.jcraft.jsch.Logger.WARN:
        return Level.WARNING;
      case com.jcraft.jsch.Logger.ERROR:
      case com.jcraft.jsch.Logger.FATAL:
        return Level.SEVERE;
      default:
        return Level.FINER;
    }
  }
}
