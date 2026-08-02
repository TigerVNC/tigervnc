package com.jcraft.jsch;

import com.jcraft.jsch.annotations.SuppressForbiddenApi;

final class JavaThreadId {

  @SuppressForbiddenApi("jdk-deprecated")
  static long get() {
    return Thread.currentThread().getId();
  }
}
