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

#ifndef __G_WAYLAND_SOURCE_H__
#define __G_WAYLAND_SOURCE_H__

#include <glib.h>

namespace wayland { class Display; }

class GWaylandSource {
public:
  GWaylandSource(wayland::Display* display);
  ~GWaylandSource();

  void attach(GMainContext* context);
private:
  int prepare(int* timeout);
  int check();
  int dispatch();

private:
  GSource* source;
  wayland::Display* display;
  void* tag;
  bool prepared;
  static GSourceFuncs sourceFuncs;
};

#endif // __G_WAYLAND_SOURCE_H__
