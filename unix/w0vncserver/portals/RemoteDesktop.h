/* Copyright 2025 Adam Halim for Cendio AB
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

#ifndef __REMOTE_DESKTOP_H__
#define __REMOTE_DESKTOP_H__

#include <stdint.h>

#include <vector>
#include <functional>
#include <string>

#include <gio/gio.h>

// Maximum number of buttons
#define BUTTONS 9

// evdev input event codes
#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE  0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

// Scroll wheel events
#define WHEEL_VERTICAL_DOWN    3
#define WHEEL_VERTICAL_UP      4
#define WHEEL_HORIZONTAL_LEFT  5
#define WHEEL_HORIZONTAL_RIGHT 6

// Devices types
static const int DEV_KEYBOARD =    (1 << 0);
static const int DEV_POINTER =     (1 << 1);
static const int DEV_TOUCHSCREEN = (1 << 2);

// Source types
static const int SRC_MONITOR = (1 << 0);
static const int SRC_WINDOW =  (1 << 1);
static const int SRC_VIRTUAL = (1 << 2);

// Cursor modes
static const int CUR_HIDEN =    (1 << 0);
static const int CUR_EMBEDDED = (1 << 1);
static const int CUR_METADATA = (1 << 2);

struct PipeWireStreamData {
  const char* id;
  uint32_t pwNodeID;
  uint32_t width;
  uint32_t height;
  uint32_t x;
  uint32_t y;
  uint32_t sourceType;
  const char* mappingId;
};

namespace rfb { class VNCServer; }
class Portal;

class RemoteDesktop {
public:
  RemoteDesktop(std::function<void(int fd, uint32_t nodeId)> startPipewireCb);
  ~RemoteDesktop();

  // Called from SDesktop
  void keyEvent(uint32_t keysym, uint32_t keycode, bool down);
  void pointerEvent(int32_t x, int32_t y, uint16_t buttonMask);

  void createSession();
private:
  void closeSession();
  void selectDevices();
  void selectSources();
  void start();
  void openPipewireRemote();

  void startPipewire(int fd, uint32_t nodeId);

  // Portal signal callbacks
  void handleCreateSession(GVariant *parameters);
  void handleStart(GVariant *parameters);
  void handleSelectDevices(GVariant *parameters);
  void handleSelectSources(GVariant *parameters);
  void handleOpenPipewireRemote(GObject *proxy, GAsyncResult *res);

  // pointerEvent help functions
  void handleButton(int32_t button, bool down);
  void handleScrollWheel(int32_t button);

  // Parses ScreenCast streams. Returns false on error
  bool parseStreams(GVariant* streams);
  PipeWireStreamData* parseStream(GVariant* stream);

  std::vector<PipeWireStreamData*> pipewireStreams() { return streams; }

private:
  uint16_t oldButtonMask;
  uint32_t selectedDevices;
  bool pipewireStarted;
  std::string sessionHandle;

  std::vector<PipeWireStreamData*> streams;

  Portal* remoteDesktopProxy;
  Portal* screencastProxy;

  std::function<void(int fd, uint32_t nodeId)> startPipewireCb;
};

#endif // __REMOTE_DESKTOP_H__
