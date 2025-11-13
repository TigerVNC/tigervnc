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

#ifndef __WAYLAND_DESKTOP_H__
#define __WAYLAND_DESKTOP_H__

#include <glib.h>
#include <stdint.h>

#include <rfb/SDesktop.h>

namespace rfb { class VNCServer; }

namespace wayland {
  class Output;
  class Display;
  class Seat;
  class VirtualPointer;
  class VirtualKeyboard;
  class DataControl;
};

class WaylandPixelBuffer;
class GWaylandSource;

class WaylandDesktop : public rfb::SDesktop {
public:
  WaylandDesktop(GMainLoop* loop);
  virtual ~WaylandDesktop();

  // // -=- SDesktop interface
  void init(rfb::VNCServer* vs) override;
  void start() override;
  virtual void stop() override;
  virtual void keyEvent(uint32_t keysym, uint32_t keycode,
                        bool down) override;
  virtual void pointerEvent(const core::Point& pos,
                            uint16_t buttonMask) override;
  void queryConnection(network::Socket* sock,
                        const char* userName) override;
  void terminate() override;

  virtual void handleClipboardRequest() override;
  virtual void handleClipboardAnnounce(bool available) override;
  virtual void handleClipboardData(const char* data) override;

  // Check if necessary wayland protocols are available
  static bool available();

private:
  void setLEDState(unsigned int state);

protected:
  rfb::VNCServer* server;

private:
  uint16_t oldButtonMask;
  WaylandPixelBuffer* pb;
  GMainLoop* loop;
  GWaylandSource* waylandSource;
  wayland::Display* display;
  wayland::Seat* seat;
  wayland::Output* output;
  wayland::VirtualPointer* virtualPointer;
  wayland::VirtualKeyboard* virtualKeyboard;
  wayland::DataControl* dataControl;
};
#endif // __WAYLAND_DESKTOP_H__
