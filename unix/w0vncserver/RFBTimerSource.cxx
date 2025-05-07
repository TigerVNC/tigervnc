#include "RFBTimerSource.h"
#include <glib.h>

#include <assert.h>
#include <rfb/ServerCore.h>
#include <core/Timer.h>

RFBTimerSource::RFBTimerSource()
{
  source_funcs_.prepare = RFBTimerSource::prepare;
  source_funcs_.check = RFBTimerSource::check;
  source_funcs_.dispatch = RFBTimerSource::dispatch;

  source_ = g_source_new(&source_funcs_, sizeof(GSource));
}

RFBTimerSource::~RFBTimerSource() {
  assert(source_);

  g_source_destroy(source_);
}

void RFBTimerSource::attach(GMainContext* context) {
  assert(source_);

  g_source_attach(source_, context);
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
                             gpointer user_data)
{
  (void)source;
  (void)callback;
  (void)user_data;

  core::Timer::checkTimeouts();

  return true;
}
