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

#ifndef __WAYLAND_OUTPUT_H__
#define __WAYLAND_OUTPUT_H__

#include <string>

#include "Object.h"

struct wl_output;
struct wl_output_listener;

namespace wayland {
  class Display;

  struct Mode {
    uint32_t flags;
    int32_t width;
    int32_t height;
    int32_t refresh;
  };

  class Output : public Object {
  public:
    Output(Display* display);
    ~Output();

    wl_output* getOutput() const { return output; }
    uint32_t getWidth() const { return mode.width; }
    uint32_t getHeight() const { return mode.height; }

  private:
    void handleGeometry(int32_t x, int32_t y, int32_t physical_width,
                        int32_t physical_height, int32_t subpixel,
                        const char* make, const char* model,
                        int32_t transform);
    void handleMode(uint32_t flags, int32_t width, int32_t height,
                    int32_t refresh);
    void handleDone();
    void handleScale(int32_t factor);
    void handleName(const char* name);
    void handleDescription(const char* description);

  private:
    wl_output* output;
    Mode mode;
    std::string name;
    std::string description;
    static const wl_output_listener listener;
  };
};

#endif // __WAYLAND_OUTPUT_H__
