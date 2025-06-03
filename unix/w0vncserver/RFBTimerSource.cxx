#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <glib.h>

#include <rfb/ServerCore.h>
#include <core/Timer.h>

#include "RFBTimerSource.h"

RFBTimerSource::RFBTimerSource()
{
  sourceFuncs.prepare = RFBTimerSource::prepare;
  sourceFuncs.check = RFBTimerSource::check;
  sourceFuncs.dispatch = RFBTimerSource::dispatch;

  source = g_source_new(&sourceFuncs, sizeof(GSource));
}

RFBTimerSource::~RFBTimerSource() {
  assert(source);

  g_source_destroy(source);
}

void RFBTimerSource::attach(GMainContext* context) {
  assert(source);

  g_source_attach(source, context);
}

int RFBTimerSource::prepare(GSource* source, gint* timeout) {
  (void)source;
  int nextTimeout;

  *timeout = 1000/ rfb::Server::frameRate;

  nextTimeout = core::Timer::getNextTimeout();

  if (nextTimeout >= 0 && (*timeout == -1 || nextTimeout < *timeout)) {
    *timeout = nextTimeout;
  }

  return false;
}

int RFBTimerSource::check(GSource* source)
{
  (void)source;
  int nextTimeout;

  nextTimeout = core::Timer::getNextTimeout();

  if (nextTimeout < 0)
    return false;

  return true;
}

int RFBTimerSource::dispatch(GSource* source,
                             GSourceFunc callback,
                             void* userData)
{
  (void)source;
  (void)callback;
  (void)userData;

  core::Timer::checkTimeouts();

  return true;
}
