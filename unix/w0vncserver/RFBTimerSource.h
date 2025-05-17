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
  static int dispatch(GSource* source, GSourceFunc callback, void* user_data);
  static int callback(gpointer user_data);

private:
  GSource* source_;
  GSourceFuncs source_funcs_;
};
#endif