#ifndef __RFB_TIMER_SOURCE_H__
#define __RFB_TIMER_SOURCE_H__

#include <glib.h>

class RFBTimerSource {
public:
  RFBTimerSource();
  ~RFBTimerSource();

  void attach(GMainContext* context);
private:
  static int prepare(GSource* source, gint* timeout);
  static int check(GSource* source);
  static int dispatch(GSource* source, GSourceFunc callback, void* userData);
  static int callback(void* userData);

private:
  GSource* source;
  GSourceFuncs sourceFuncs;
};
#endif
