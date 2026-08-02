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
  private static Thread reactorThread;
  private static volatile boolean running = false;

  private static synchronized void initSelector() {
    if (selector == null) {
      try {
        selector = Selector.open();
        running = true;
        reactorThread = new Thread(Fl::runLoop, "FLTK Reactor Loop");
        reactorThread.setDaemon(true);
        reactorThread.start();
      } catch (IOException e) {
        throw new RuntimeException("Failed to initialize FLTK Selector reactor", e);
      }
    }
  }

  public static void add_fd(SelectableChannel channel, int when, Fl_FD_Handler cb, Object arg) {
    if (channel == null) return;
    initSelector();

    try {
      channel.configureBlocking(false);
      selector.wakeup();
      synchronized (selector) {
        int ops = 0;
        if ((when & READ) != 0) ops |= SelectionKey.OP_READ;
        if ((when & WRITE) != 0) ops |= SelectionKey.OP_WRITE;

        Registration reg = new Registration(channel, when, cb, arg);
        channel.register(selector, ops, reg);
      }
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
      selector.wakeup();
      synchronized (selector) {
        SelectionKey key = channel.keyFor(selector);
        if (key != null) {
          key.cancel();
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }

  private static void runLoop() {
    while (running) {
      try {
        if (selector.select() == 0)
          continue;

        synchronized (selector) {
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

              final int maskFinal = mask;
              reg.cb.handle(reg.channel, maskFinal, reg.arg);
            }
          }
        }
      } catch (ClosedSelectorException e) {
        break;
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
  }
}
