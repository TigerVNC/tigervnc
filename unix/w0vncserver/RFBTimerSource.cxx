#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <glib.h>

#include <core/Timer.h>

#include "RFBTimerSource.h"

static int prepare(GSource* source, gint* timeout);
static int check(GSource* source);
static int dispatch(GSource* source, GSourceFunc callback, void* userData);

RFBTimerSource::RFBTimerSource()
{
  sourceFuncs.prepare = prepare;
  sourceFuncs.check = check;
  sourceFuncs.dispatch = dispatch;

  source = g_source_new(&sourceFuncs, sizeof(GSource));
}

RFBTimerSource::~RFBTimerSource()
{
  assert(source);

  g_source_destroy(source);
}

void RFBTimerSource::attach(GMainContext* context)
{
  assert(source);

  g_source_attach(source, context);
}

int prepare(GSource* source, gint* timeout)
{
  (void)source;
  int nextTimeout;

  nextTimeout = core::Timer::getNextTimeout();

  if (nextTimeout >= 0 && (*timeout == -1 || nextTimeout < *timeout))
    *timeout = nextTimeout;

  return FALSE;
}

int check(GSource* source)
{
  (void)source;
  int nextTimeout;

  nextTimeout = core::Timer::getNextTimeout();

  if (nextTimeout < 0)
    return FALSE;

  return TRUE;
}

int dispatch(GSource* source, GSourceFunc callback, void* userData)
{
  (void)source;
  (void)callback;
  (void)userData;

  core::Timer::checkTimeouts();

  return G_SOURCE_CONTINUE;
}
