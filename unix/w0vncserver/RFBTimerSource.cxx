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

#include <core/Timer.h>

#include "w0vncserver.h"
#include "RFBTimerSource.h"

static std::map<GSource*, RFBTimerSource*> sources;

GSourceFuncs RFBTimerSource::sourceFuncs {
  .prepare = [](GSource* source, int* timeout) {
    assert(sources.count(source) != 0);
    return sources[source]->prepare(timeout);
  },
  .check = [](GSource* source) {
    assert(sources.count(source) != 0);
    return sources[source]->check();
  },
  .dispatch = [](GSource* source, GSourceFunc, void*) {
    assert(sources.count(source) != 0);
    return sources[source]->dispatch();
  },
  .finalize = nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

RFBTimerSource::RFBTimerSource()
{
  source = g_source_new(&sourceFuncs, sizeof(GSource));
  sources[source] = this;
}

RFBTimerSource::~RFBTimerSource()
{
  sources.erase(source);
  g_source_unref(source);
  g_source_destroy(source);
}

void RFBTimerSource::attach(GMainContext* context)
{
  assert(source);

  g_source_attach(source, context);
}

int RFBTimerSource::prepare(int* timeout)
{
  int nextTimeout;

  *timeout = -1;

  nextTimeout = core::Timer::getNextTimeout();

  if (nextTimeout >= 0)
    *timeout = nextTimeout;

  return FALSE;
}

int RFBTimerSource::check()
{
  int nextTimeout;

  nextTimeout = core::Timer::getNextTimeout();

  if (nextTimeout < 0)
    return FALSE;

  return TRUE;
}

int RFBTimerSource::dispatch()
{
  core::Timer::checkTimeouts();

  return G_SOURCE_CONTINUE;
}
