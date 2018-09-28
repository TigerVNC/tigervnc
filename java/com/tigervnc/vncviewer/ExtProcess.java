/*
 *  Copyright (C) 2016 Brian P. Hinz. All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 *  USA.
 */

package com.tigervnc.vncviewer;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;
import com.tigervnc.network.*;

import static com.tigervnc.vncviewer.Parameters.*;

public class ExtProcess implements Runnable {

  private String cmd = null;
  private LogWriter vlog = null;
  private boolean shutdown = false;
  private Process pid = null;

  private static class MyProcessLogger extends Thread {
    private final BufferedReader err;
    private final LogWriter vlog;

    public MyProcessLogger(Process p, LogWriter vlog) {
      InputStreamReader reader = 
        new InputStreamReader(p.getErrorStream());
      err = new BufferedReader(reader);
      this.vlog = vlog;
    }

    @Override
    public void run() {
      try {
        while (true) {
          String msg = err.readLine();
          if (msg != null)
            vlog.info(msg);
        }
      } catch(java.io.IOException e) {
        vlog.info(e.getMessage());
      } finally {
        try {
          if (err != null)
            err.close();
        } catch (java.io.IOException e ) { }
      }
    }
  }

  private static class MyShutdownHook extends Thread {

    private Process proc = null;

    public MyShutdownHook(Process p) {
      proc = p;
    }

    @Override
    public void run() {
      try {
        proc.exitValue();
      } catch (IllegalThreadStateException e) {
        try {
          // wait for CConn to shutdown the socket
          Thread.sleep(500);
        } catch(InterruptedException ie) { }
        proc.destroy();
      }
    }
  }

  public ExtProcess(String command, LogWriter vlog, boolean shutdown) {
    cmd = command;  
    this.vlog = vlog;
    this.shutdown = shutdown;
  }

  public ExtProcess(String command, LogWriter vlog) {
    this(command, vlog, false);
  }

  public ExtProcess(String command) {
    this(command, null, false);
  }

  public void run() {
    try {
      Runtime runtime = Runtime.getRuntime();
      pid = runtime.exec(cmd);
      if (shutdown)
        runtime.addShutdownHook(new MyShutdownHook(pid));
      if (vlog != null)
        new MyProcessLogger(pid, vlog).start();
      pid.waitFor();
    } catch(InterruptedException e) {
      vlog.info(e.getMessage());
    } catch(java.io.IOException e) {
      vlog.info(e.getMessage());
    }
  }

  //static LogWriter vlog = new LogWriter("ExtProcess");
}
