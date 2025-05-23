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

#include <map>

#include <pipewire/pipewire.h>
#include <pipewire/stream.h>

#include <core/LogWriter.h>

#include "PipeWireSource.h"
#include "w0vncserver.h"

static core::LogWriter vlog("PipewireSource");

static int pwInitCount = 0;

static std::map<GSource*, PipeWireSource*> sources;

GSourceFuncs PipeWireSource::pipewireSourceFuncs {
  .prepare = nullptr,
  .check = nullptr,
  .dispatch = [](GSource* source, GSourceFunc, void*) {
    assert(sources.count(source) != 0);
    return sources[source]->sourceLoopDispatch();
  },
  .finalize =nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

PipeWireSource::PipeWireSource()
{
  // pw_init() and pw_deinit() may only be called once prior to version
  // 0.3.49.
  if (pwInitCount++ == 0)
    pw_init(nullptr, nullptr);

  loop = pw_loop_new(nullptr);
  pw_loop_enter(loop);
  source = g_source_new(&pipewireSourceFuncs, sizeof(GSource));
  g_source_add_unix_fd(source, pw_loop_get_fd (loop),
                       (GIOCondition)(G_IO_IN | G_IO_ERR));
  g_source_attach (source, nullptr);
  sources[source] = this;
}

PipeWireSource::~PipeWireSource()
{
  sources.erase(source);
  g_source_unref(source);
  g_source_destroy(source);

  pw_loop_leave(loop);
  pw_loop_destroy(loop);

  assert(pwInitCount > 0);
  if (--pwInitCount == 0)
    pw_deinit();
}

int PipeWireSource::sourceLoopDispatch()
{
  int result;

  result = pw_loop_iterate (loop, 0);
  if (result < 0)
    vlog.error("pipewire_loop_iterate failed: %s", g_strerror (result));

  return G_SOURCE_CONTINUE;
}
