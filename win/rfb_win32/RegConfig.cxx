/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <malloc.h>

#include <core/LogWriter.h>

#include <rfb_win32/RegConfig.h>

//#include <rdr/HexOutStream.h>

using namespace rfb;
using namespace rfb::win32;


static core::LogWriter vlog("RegConfig");


RegConfig::RegConfig(EventManager* em)
  : eventMgr(em), event(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
    callback(nullptr)
{
  if (em->addEvent(event, this))
    eventMgr = em;
}

RegConfig::~RegConfig() {
  if (eventMgr)
    eventMgr->removeEvent(event);
}

bool RegConfig::setKey(const HKEY rootkey, const char* keyname) {
  try {
    key.createKey(rootkey, keyname);
    processEvent(event);
    return true;
  } catch (std::exception& e) {
    vlog.debug("%s", e.what());
    return false;
  }
}

void RegConfig::loadRegistryConfig(RegKey& key) {
  DWORD i = 0;
  try {
    while (1) {
      const char *name = key.getValueName(i++);
      if (!name) break;
      std::string value = key.getRepresentation(name);
      if (!core::Configuration::setParam(name, value.c_str()))
        vlog.info("Unable to process %s", name);
    }
  } catch (core::win32_error& e) {
    if (e.err != ERROR_INVALID_HANDLE)
      vlog.error("%s", e.what());
  }
}

void RegConfig::processEvent(HANDLE /*event*/) {
  vlog.info("Registry changed");

  // Reinstate the registry change notifications
  ResetEvent(event);
  key.awaitChange(true, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, event);

  // Load settings
  loadRegistryConfig(key);

  // Notify the callback, if supplied
  if (callback)
    callback->regConfigChanged();
}


RegConfigThread::RegConfigThread() : config(&eventMgr), thread_id(-1) {
}

RegConfigThread::~RegConfigThread() {
  PostThreadMessage(thread_id, WM_QUIT, 0, 0);
  wait();
}

bool RegConfigThread::start(const HKEY rootKey, const char* keyname) {
  if (config.setKey(rootKey, keyname)) {
    Thread::start();
    while (thread_id == (DWORD)-1)
      Sleep(0);
    return true;
  }
  return false;
}

void RegConfigThread::worker() {
  BOOL result = 0;
  MSG msg;
  thread_id = GetCurrentThreadId();
  while ((result = eventMgr.getMessage(&msg, nullptr, 0, 0)) > 0) {}
  if (result < 0)
    throw core::win32_error("RegConfigThread failed", GetLastError());
}
