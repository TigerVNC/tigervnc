/*
 * Copyright (c) 2011 ymnk, JCraft,Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3. The names of the authors may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JCRAFT, INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.jcraft.jsch;

import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.sun.jna.platform.win32.BaseTSD.ULONG_PTR;
import com.sun.jna.platform.win32.Kernel32;
import com.sun.jna.platform.win32.User32;
import com.sun.jna.platform.win32.WinBase;
import com.sun.jna.platform.win32.WinBase.SECURITY_ATTRIBUTES;
import com.sun.jna.platform.win32.WinDef.HWND;
import com.sun.jna.platform.win32.WinDef.LPARAM;
import com.sun.jna.platform.win32.WinDef.LRESULT;
import com.sun.jna.platform.win32.WinError;
import com.sun.jna.platform.win32.WinNT;
import com.sun.jna.platform.win32.WinNT.HANDLE;
import com.sun.jna.platform.win32.WinUser;
import com.sun.jna.platform.win32.WinUser.COPYDATASTRUCT;
import java.util.Locale;

public class PageantConnector implements AgentConnector {

  private static final int AGENT_MAX_MSGLEN = 262144;
  private static final long AGENT_COPYDATA_ID = 0x804e50baL;

  private final User32 user32;
  private final Kernel32 kernel32;

  public PageantConnector() throws AgentProxyException {
    if (!Util.getSystemProperty("os.name", "").startsWith("Windows")) {
      throw new AgentProxyException("PageantConnector only available on Windows.");
    }

    try {
      user32 = User32.INSTANCE;
      kernel32 = Kernel32.INSTANCE;
    } catch (LinkageError e) {
      throw new AgentProxyException(e.toString(), e);
    }
  }

  @Override
  public String getName() {
    return "pageant";
  }

  @Override
  public boolean isAvailable() {
    return user32.FindWindow("Pageant", "Pageant") != null;
  }

  @Override
  public void query(Buffer buffer) throws AgentProxyException {
    if (buffer.getLength() > AGENT_MAX_MSGLEN) {
      throw new AgentProxyException("Query too large.");
    }

    HWND hwnd = user32.FindWindow("Pageant", "Pageant");

    if (hwnd == null) {
      throw new AgentProxyException("Pageant is not runnning.");
    }

    String threadId = JavaVersion.getVersion() >= 19
        ? String.format(Locale.ROOT, "%08x%08x", kernel32.GetCurrentProcessId(), JavaThreadId.get())
        : String.format(Locale.ROOT, "%08x", kernel32.GetCurrentThreadId());
    String mapname = "JSchPageantRequest" + threadId;

    HANDLE sharedFile = null;
    Pointer sharedMemory = null;
    try {
      // TODO
      SECURITY_ATTRIBUTES psa = null;

      sharedFile = kernel32.CreateFileMapping(WinBase.INVALID_HANDLE_VALUE, psa,
          WinNT.PAGE_READWRITE, 0, AGENT_MAX_MSGLEN, mapname);
      int lastError = kernel32.GetLastError();
      if (sharedFile == null) {
        throw new AgentProxyException(
            "Unable to CreateFileMapping(): GetLastError() = " + lastError);
      }
      if (lastError == WinError.ERROR_ALREADY_EXISTS) {
        throw new AgentProxyException("Shared file mapping already exists");
      }

      sharedMemory = kernel32.MapViewOfFile(sharedFile, WinBase.FILE_MAP_WRITE, 0, 0, 0);
      if (sharedMemory == null) {
        throw new AgentProxyException(
            "Unable to MapViewOfFile(): GetLastError() = " + kernel32.GetLastError());
      }

      sharedMemory.write(0, buffer.buffer, 0, buffer.getLength());

      COPYDATASTRUCT cds = createCDS(mapname);
      long rcode = sendMessage(hwnd, cds);
      // Dummy read to make sure COPYDATASTRUCT isn't GC'd early
      long foo = cds.dwData.longValue();

      buffer.rewind();
      if (rcode != 0) {
        sharedMemory.read(0, buffer.buffer, 0, 4); // length
        int i = buffer.getInt();
        if (i <= 0 || i > AGENT_MAX_MSGLEN - 4) {
          throw new AgentProxyException("Illegal length: " + i);
        }
        buffer.rewind();
        buffer.checkFreeSize(i);
        sharedMemory.read(4, buffer.buffer, 0, i);
      } else {
        throw new AgentProxyException(
            "SendMessage() returned 0 with cds.dwData: " + Long.toHexString(foo));
      }
    } finally {
      try {
        if (sharedMemory != null) {
          kernel32.UnmapViewOfFile(sharedMemory);
        }
      } finally {
        if (sharedFile != null) {
          kernel32.CloseHandle(sharedFile);
        }
      }
    }
  }

  static COPYDATASTRUCT createCDS(String mapname) {
    Memory foo = new Memory(mapname.length() + 1);
    foo.setString(0, mapname, "US-ASCII");
    COPYDATASTRUCT cds = new COPYDATASTRUCT();
    cds.dwData = new ULONG_PTR(AGENT_COPYDATA_ID);
    cds.cbData = (int) foo.size();
    cds.lpData = foo;
    cds.write();
    return cds;
  }

  long sendMessage(HWND hwnd, COPYDATASTRUCT cds) {
    LPARAM data = new LPARAM(Pointer.nativeValue(cds.getPointer()));
    LRESULT result = user32.SendMessage(hwnd, WinUser.WM_COPYDATA, null, data);
    return result.longValue();
  }
}
