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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <wayland-client-protocol.h>

#include <core/LogWriter.h>

#include "../../w0vncserver.h"
#include "Display.h"
#include "Object.h"
#include "Output.h"

using namespace wayland;

const wl_output_listener Output::listener = {
  .geometry = [](void* data, wl_output*, int32_t x, int32_t y,
                 int32_t physicalWidth, int32_t physicalHeight,
                 int32_t subpixel, const char* make, const char* model,
                 int32_t transform) {
    ((Output*)data)->handleGeometry(x, y, physicalWidth, physicalHeight,
                                    subpixel, make, model, transform);
  },
  .mode = [](void* data, wl_output*, uint32_t flags, int32_t width,
             int32_t height, int32_t refresh) {
    ((Output*)data)->handleMode(flags, width, height, refresh);
  },
  .done = [](void* data, wl_output*) {
    ((Output*)data)->handleDone();
  },
  .scale = [](void* data, wl_output*, int32_t factor) {
    ((Output*)data)->handleScale(factor);
  },
  .name = [](void* data, wl_output*, const char* name) {
    ((Output*)data)->handleName(name);
  },
  .description = [](void* data, wl_output*, const char* description) {
    ((Output*)data)->handleDescription(description);
  }
};

static core::LogWriter vlog("WOutput");

Output::Output(Display* display)
  : Object(display, "wl_output", &wl_output_interface), output(nullptr),
    mode({0, 0, 0, 0})
{
  output = (wl_output*) boundObject;

  wl_output_add_listener(output, &listener, this);
  display->roundtrip();
}

Output::~Output()
{
  wl_output_destroy(output);
}

void Output::handleGeometry(int32_t /* x */, int32_t /* y */,
                            int32_t /* physical_width */,
                            int32_t /* physical_height */,
                            int32_t /* subpixel */,
                            const char* /* make */,
                            const char*  /* model */,
                            int32_t /* transform */)
{
  // FIXME: This gives us the position/size etc. of the output
  // (presumably a screen). Keep track of this when we want to
  // handle multiple monitors.
}

void Output::handleMode(uint32_t flags, int32_t width, int32_t height,
                        int32_t refresh)
{
  if (mode.width != width || mode.height != height) {
    vlog.debug("Resized from %dx%d to %dx%d", mode.width, mode.height,
                width, height);
  }

  // FIXME: Handle multiple screens
  // FIXME: The flags "describe properties of an output mode".
  //        possible values are 0x1 - current
  //                            0x2 - preferred
  //        Don't think we care?
  mode = {
    .flags = flags,
    .width = width,
    .height = height,
    .refresh = refresh,
  };
}

void Output::handleDone()
{
  // All properties have been sent.
}

void Output::handleScale(int32_t /* factor */)
{
  // FIXME: Handle scaling
}

void Output::handleName(const char* /* name */)
{
  // Output name, e.g. "HDMI-1", "WL-1" etc.
}

void Output::handleDescription(const char* /* description */)
{
  // Optional description, e.g. "Dell inc. 24"
}
