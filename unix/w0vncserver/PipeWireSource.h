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

#ifndef __PIPEWIRE_SOURCE_H__
#define __PIPEWIRE_SOURCE_H__

#include <glib.h>
#include <pipewire/stream.h>


class PipeWireSource {
public:
  PipeWireSource();
  ~PipeWireSource();

  pw_loop* getLoop() const { return loop; }

private:
  int  sourceLoopDispatch();
  void handleStreamStateChanged(enum pw_stream_state old,
                                enum pw_stream_state state,
                                const char* error);
  void handleStreamParamChanged(uint32_t id, const spa_pod* param);
  void handleProcess();

private:
  pw_loop* loop;
  GSource* source;
  pw_stream* stream;
  int nodeId;
  static GSourceFuncs pipewireSourceFuncs;
};

#endif // __PIPEWIRE_SOURCE_H__
