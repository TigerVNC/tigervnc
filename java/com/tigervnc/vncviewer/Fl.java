/*
  * Copyright (C) 2026 TigerVNC Team
  *
  * This is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This software is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  */

package com.tigervnc.vncviewer;

import java.io.IOException;
import java.nio.channels.ClosedSelectorException;
import java.nio.channels.SelectableChannel;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.util.Iterator;

public class Fl {

  public static final int READ = SelectionKey.OP_READ;
  public static final int WRITE = SelectionKey.OP_WRITE;
  public static final int EXCEPT = 0;

  @FunctionalInterface
  public interface Fl_FD_Handler {
    void handle(SelectableChannel channel, int mask, Object arg);
  }

  private static class Registration {
    SelectableChannel channel;
    int when;
    Fl_FD_Handler cb;
    Object arg;

    Registration(SelectableChannel channel, int when, Fl_FD_Handler cb, Object arg) {
      this.channel = channel;
      this.when = when;
      this.cb = cb;
      this.arg = arg;
    }
  }

  private static Selector selector;

  private static synchronized void initSelector() {
    if (selector == null) {
      try {
        selector = Selector.open();
      } catch (IOException e) {
        throw new RuntimeException("Failed to initialize FLTK Selector reactor", e);
      }
    }
  }

  public static double wait(double time) {
    initSelector();
    long timeout = (long)(time * 1000.0);

    try {
      int n;
      if (timeout <= 0) {
        n = selector.selectNow();
      } else {
        n = selector.select(timeout);
      }
      if (n > 0) {
        Iterator<SelectionKey> keys = selector.selectedKeys().iterator();
        while (keys.hasNext()) {
          SelectionKey key = keys.next();
          keys.remove();

          if (!key.isValid())
            continue;

          Registration reg = (Registration) key.attachment();
          if (reg != null && reg.cb != null) {
            int readyOps = key.readyOps();
            int mask = 0;
            if ((readyOps & SelectionKey.OP_READ) != 0) mask |= READ;
            if ((readyOps & SelectionKey.OP_WRITE) != 0) mask |= WRITE;

            reg.cb.handle(reg.channel, mask, reg.arg);
          }
        }
      }
    } catch (ClosedSelectorException e) {
      return -1.0;
    } catch (IOException e) {
      return -1.0;
    }
    return 0.0;
  }

  public static void add_fd(SelectableChannel channel, int when, Fl_FD_Handler cb, Object arg) {
    if (channel == null) return;
    initSelector();

    try {
      channel.configureBlocking(false);
      int ops = 0;
      if ((when & READ) != 0) ops |= SelectionKey.OP_READ;
      if ((when & WRITE) != 0) ops |= SelectionKey.OP_WRITE;

      Registration reg = new Registration(channel, when, cb, arg);
      SelectionKey key = channel.keyFor(selector);
      if (key != null && key.isValid()) {
        key.interestOps(ops);
        key.attach(reg);
      } else {
        channel.register(selector, ops, reg);
      }
      selector.wakeup();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public static void add_fd(SelectableChannel channel, int when, Fl_FD_Handler cb) {
    add_fd(channel, when, cb, null);
  }

  public static void remove_fd(SelectableChannel channel) {
    if (channel == null || selector == null) return;

    try {
      SelectionKey key = channel.keyFor(selector);
      if (key != null && key.isValid()) {
        key.interestOps(0);
      }
      selector.wakeup();
    } catch (Exception e) {
      e.printStackTrace();
    }
  }
}
