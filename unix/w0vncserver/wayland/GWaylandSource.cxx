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

#include <glib.h>

#include <wayland-client-core.h>

#include <core/LogWriter.h>

#include "objects/Display.h"
#include "GWaylandSource.h"

using namespace wayland;

static core::LogWriter vlog("GWaylandSource");

static std::map<GSource*, GWaylandSource*> sources;

GSourceFuncs GWaylandSource::sourceFuncs {
  .prepare = [](GSource* source, int* timeout) {
    assert(sources.count(source) != 0);
    return sources[source]->prepare(timeout);
  },
  .check = [](GSource* source) {
    assert(sources.count(source) != 0);
    return sources[source]->check();
  },
  .dispatch = [](GSource* source, GSourceFunc,
                 void*) {
    assert(sources.count(source) != 0);
    return sources[source]->dispatch();
  },
  .finalize = nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

GWaylandSource::GWaylandSource(Display* display_)
  : source(nullptr), display(display_), tag(nullptr)
{
  int fd;
  GIOCondition conditions;

  source = g_source_new(&sourceFuncs, sizeof(GSource));

  conditions = (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR);
  fd = wl_display_get_fd(display->getDisplay());

  tag = g_source_add_unix_fd(source, fd, conditions);
  prepared = false;

  sources[source] = this;
}

GWaylandSource::~GWaylandSource() {
  sources.erase(source);
  if (source && prepared)
    wl_display_cancel_read(display->getDisplay());
  if (source)
    g_source_destroy(source);
}

void GWaylandSource::attach(GMainContext* context) {
  g_source_attach(source, context);
}

int GWaylandSource::prepare(int* timeout)
{
  wl_display_flush(display->getDisplay());

  *timeout = -1;

  if (prepared)
    return FALSE;

  // We only want to call wl_display_prepare_read() once
  prepared = true;

  if (wl_display_prepare_read(display->getDisplay()) != 0) {
    if (wl_display_dispatch_pending(display->getDisplay()) < 0) {
      // FIXME: Stop here?
      vlog.error("Failed to flush wl_display: %s", strerror(errno));
    }
  }

  return FALSE;
}

int GWaylandSource::check()
{
  return g_source_query_unix_fd(source, tag) > 0;
}

int GWaylandSource::dispatch()
{
  GIOCondition events;

  events = g_source_query_unix_fd(source, tag);

  assert(prepared);

  if (events & G_IO_IN)
    wl_display_read_events(display->getDisplay());
  if (events & G_IO_HUP || events & G_IO_ERR)
    wl_display_cancel_read(display->getDisplay());

  wl_display_dispatch_pending(display->getDisplay());
  prepared = false;

  return G_SOURCE_CONTINUE;
}
