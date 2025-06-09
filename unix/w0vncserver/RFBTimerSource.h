#ifndef __RFB_TIMER_SOURCE_H__
#define __RFB_TIMER_SOURCE_H__

#include <glib.h>

class RFBTimerSource {
public:
  RFBTimerSource();
  ~RFBTimerSource();

  void attach(GMainContext* context);
private:
  GSource* source;
  GSourceFuncs sourceFuncs;
};
#endif
