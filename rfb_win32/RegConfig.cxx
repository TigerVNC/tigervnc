/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- RegConfig.cxx

#include <malloc.h>

#include <rfb_win32/RegConfig.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rdr/HexOutStream.h>

using namespace rfb;
using namespace rfb::win32;


static LogWriter vlog("RegConfig");


class rfb::win32::RegReaderThread : public Thread {
public:
  RegReaderThread(RegistryReader& reader, const HKEY key);
  ~RegReaderThread();
  virtual void run();
  virtual Thread* join();
protected:
  RegistryReader& reader;
  RegKey key;
  HANDLE event;
};

RegReaderThread::RegReaderThread(RegistryReader& reader_, const HKEY key_) : Thread("RegConfig"), reader(reader_), key(key_) {
}

RegReaderThread::~RegReaderThread() {
}

void
RegReaderThread::run() {
  vlog.debug("RegReaderThread started");
  while (key) {
    // - Wait for changes
    vlog.debug("waiting for changes");
    key.awaitChange(true, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET);

    // - Load settings
    RegistryReader::loadRegistryConfig(key);

    // - Notify specified thread of changes
    if (reader.notifyThread)
      PostThreadMessage(reader.notifyThread->getThreadId(),
        reader.notifyMsg.message, 
        reader.notifyMsg.wParam, 
        reader.notifyMsg.lParam);
    else if (reader.notifyWindow)
      PostMessage(reader.notifyWindow,
        reader.notifyMsg.message, 
        reader.notifyMsg.wParam, 
        reader.notifyMsg.lParam);
  }
}

Thread*
RegReaderThread::join() {
  RegKey old_key = key;
  key.close();
  if ((HKEY)old_key) {
    // *** Closing the key doesn't always seem to work
    //     Writing to it always will, instead...
    vlog.debug("closing key");
    old_key.setString(_T("dummy"), _T(""));
  }
  return Thread::join();
}


RegistryReader::RegistryReader() : thread(0), notifyThread(0) {
  memset(&notifyMsg, 0, sizeof(notifyMsg));
}

RegistryReader::~RegistryReader() {
  if (thread) delete thread->join();
}

bool RegistryReader::setKey(const HKEY rootkey, const TCHAR* keyname) {
  if (thread) delete thread->join();
  thread = 0;
  
  RegKey key;
  try {
    key.createKey(rootkey, keyname);
    loadRegistryConfig(key);
  } catch (rdr::Exception& e) {
    vlog.debug(e.str());
    return false;
  }
  thread = new RegReaderThread(*this, key);
  if (thread) thread->start();
  return true;
}

void
RegistryReader::loadRegistryConfig(RegKey& key) {
  DWORD i = 0;
  try {
    while (1) {
      TCharArray name = tstrDup(key.getValueName(i++));
      if (!name.buf) break;
      TCharArray value = key.getRepresentation(name.buf);
      if (!value.buf || !Configuration::setParam(CStr(name.buf), CStr(value.buf)))
        vlog.info("unable to process %s", CStr(name.buf));
    }
  } catch (rdr::SystemException& e) {
    if (e.err != 6)
      vlog.error(e.str());
  }
}

bool RegistryReader::setNotifyThread(Thread* thread, UINT winMsg, WPARAM wParam, LPARAM lParam) {
  notifyMsg.message = winMsg;
  notifyMsg.wParam = wParam;
  notifyMsg.lParam = lParam;
  notifyThread = thread;
  notifyWindow = 0;
  return true;
}

bool RegistryReader::setNotifyWindow(HWND window, UINT winMsg, WPARAM wParam, LPARAM lParam) {
  notifyMsg.message = winMsg;
  notifyMsg.wParam = wParam;
  notifyMsg.lParam = lParam;
  notifyWindow = window;
  notifyThread = 0;
  return true;
}

