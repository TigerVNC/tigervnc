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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>

#include <stdexcept>

#include <rfb_win32/EventManager.h>

#include <core/Exception.h>
#include <core/LogWriter.h>

using namespace core;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("EventManager");


EventManager::EventManager() : eventCount(0) {
}

EventManager::~EventManager() {
}


bool EventManager::addEvent(HANDLE event, EventHandler* ecb) {
  if (eventCount >= MAXIMUM_WAIT_OBJECTS-1)
    return false;
  events[eventCount] = event;
  handlers[eventCount] = ecb;
  eventCount++;
  return true;
}

void EventManager::removeEvent(HANDLE event) {
  for (unsigned int i=0; i<eventCount; i++) {
    if (events[i] == event) {
      for (unsigned int j=i; j<eventCount-1; j++) {
        events[j] = events[j+1];
        handlers[j] = handlers[j+1];
      }
      eventCount--;
      return;
    }
  }
  throw std::runtime_error("Event not registered");
}


int EventManager::checkTimeouts() {
  return -1;
}

BOOL EventManager::getMessage(MSG* msg, HWND hwnd, UINT minMsg, UINT maxMsg) {
  while (true) {
    // - Process any pending timeouts
    int timeout = checkTimeouts();
    if (timeout < 0)
      timeout = INFINITE;

    // - Events take precedence over messages
    DWORD result;
    if (eventCount) {
      // - Check whether any events are set
      result = WaitForMultipleObjects(eventCount, events, FALSE, 0);
      if (result == WAIT_TIMEOUT) {
        // - No events are set, so check for messages
        if (PeekMessage(msg, hwnd, minMsg, maxMsg, PM_REMOVE)) 
          return msg->message != WM_QUIT;

        // - Block waiting for an event to be set, or a message
        result = MsgWaitForMultipleObjects(eventCount, events, FALSE, timeout,
                                           QS_ALLINPUT);
        if (result == WAIT_OBJECT_0 + eventCount) {
          // - Return the message, if any
          if (PeekMessage(msg, hwnd, minMsg, maxMsg, PM_REMOVE)) 
            return msg->message != WM_QUIT;
          continue;
        }
      }
    } else
      return GetMessage(msg, hwnd, minMsg, maxMsg);

    if (result < (WAIT_OBJECT_0 + eventCount)) {
      // - An event was set - call the handler
      int index = result - WAIT_OBJECT_0;
      handlers[index]->processEvent(events[index]);
    } else if (result == WAIT_FAILED) {
      // - An error has occurred, so return the error status code
      return -1;
    }
  }
}
