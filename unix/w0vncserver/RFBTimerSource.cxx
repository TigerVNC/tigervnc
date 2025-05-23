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

#include <stdexcept>
#include <map>

#include <glib.h>

#include <core/Timer.h>

#include "w0vncserver.h"
#include "RFBTimerSource.h"

static std::map<GSource*, RFBTimerSource*> sources;

GSourceFuncs RFBTimerSource::sourceFuncs {
  .prepare = [](GSource* source, int* timeout) {
    try {
      return sources[source]->prepare(timeout);
    } catch (std::out_of_range& e) {
      fatal_error("RFBTimerSource: %s", e.what());
      return G_SOURCE_REMOVE;
    }
  },
  .check = [](GSource* source) {
    try {
      return sources.at(source)->check();
    } catch (std::out_of_range& e) {
      fatal_error("RFBTimerSource: %s", e.what());
      return G_SOURCE_REMOVE;
    }
  },
  .dispatch = [](GSource* source, GSourceFunc, void*) {
    try {
      return sources.at(source)->dispatch();
    } catch (std::out_of_range& e) {
      fatal_error("RFBTimerSource: %s", e.what());
      return G_SOURCE_REMOVE;
    }
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
