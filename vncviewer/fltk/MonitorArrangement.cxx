/* Copyright 2021 Hugo Lundin <huglu@cendio.se> for Cendio AB.
 * Copyright 2021 Pierre Ossman for Cendio AB
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <set>
#include <vector>
#include <string>
#include <utility>
#include <sstream>
#include <assert.h>
#include <algorithm>

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Button.H>
#include <FL/x.H>

#if defined(HAVE_XRANDR) && !defined(__APPLE__)
#include <X11/extensions/Xrandr.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#endif

#include "MonitorArrangement.h"

static std::set<MonitorArrangement *> instances;
static const Fl_Boxtype FL_CHECKERED_BOX = FL_FREE_BOXTYPE;

MonitorArrangement::MonitorArrangement(
   int x, int y, int w, int h)
:  Fl_Group(x, y, w, h),
   SELECTION_COLOR(fl_lighter(FL_BLUE)),
   AVAILABLE_COLOR(fl_lighter(fl_lighter(fl_lighter(FL_BACKGROUND_COLOR))))
{
  // Used for required monitors.
  Fl::set_boxtype(FL_CHECKERED_BOX, checkered_pattern_draw, 0, 0, 0, 0);

  if (instances.size() == 0)
    Fl::add_handler(fltk_event_handler);
  instances.insert(this);

  box(FL_DOWN_BOX);
  color(fl_lighter(FL_BACKGROUND_COLOR));
  layout();
  end();
}

MonitorArrangement::~MonitorArrangement()
{
  instances.erase(this);

  if (instances.size() == 0)
    Fl::remove_handler(fltk_event_handler);
}

std::set<int> MonitorArrangement::get()
{
  std::set<int> indices;
  MonitorMap::const_iterator iter;

  for (iter = monitors.begin(); iter != monitors.end(); ++iter) {
    if (iter->second->value() == 1)
      indices.insert(iter->first);
  }

  return indices;
}

void MonitorArrangement::set(std::set<int> indices)
{
  MonitorMap::const_iterator iter;

  for (iter = monitors.begin(); iter != monitors.end(); ++iter) {
    bool selected = std::find(indices.begin(), indices.end(),
                              iter->first) != indices.end();
    iter->second->value(selected ? 1 : 0);
  }
}

void MonitorArrangement::draw()
{
  MonitorMap::const_iterator iter;

  for (iter = monitors.begin(); iter != monitors.end(); ++iter) {
    Fl_Button * monitor = iter->second;

    if (is_required(iter->first)) {
      monitor->box(FL_CHECKERED_BOX);
      monitor->color(SELECTION_COLOR);
    } else {
      monitor->box(FL_BORDER_BOX);
      monitor->color(AVAILABLE_COLOR);
      monitor->selection_color(SELECTION_COLOR);
    }
  }

  Fl_Group::draw();
}

void MonitorArrangement::layout()
{
  int x, y, w, h;
  double scale = this->scale();
  const double MARGIN_SCALE_FACTOR = 0.99;
  std::pair<int, int> offset = this->offset();

  for (int i = 0; i < Fl::screen_count(); i++) {
    bool match;

    Fl::screen_xywh(x, y, w, h, i);

    // Only keep a single entry for mirrored screens
    match = false;
    for (int j = 0; j < i; j++) {
        int x2, y2, w2, h2;

        Fl::screen_xywh(x2, y2, w2, h2, j);

        if ((x != x2) || (y != y2) || (w != w2) || (h != h2))
            continue;

        match = true;
        break;
    }
    if (match)
        continue;

    Fl_Button *monitor = new Fl_Button(
      /* x = */ this->x() + offset.first + x*scale + (1 - MARGIN_SCALE_FACTOR)*x*scale,
      /* y = */ this->y() + offset.second +  y*scale + (1 - MARGIN_SCALE_FACTOR)*y*scale,
      /* w = */ w*scale*MARGIN_SCALE_FACTOR,
      /* h = */ h*scale*MARGIN_SCALE_FACTOR
    );

    monitor->clear_visible_focus();
    monitor->callback(monitor_pressed, this);
    monitor->type(FL_TOGGLE_BUTTON);
    monitor->when(FL_WHEN_CHANGED);
    monitor->copy_tooltip(description(i).c_str());
    monitors[i] = monitor;
  }
}

void MonitorArrangement::refresh()
{
  // The selection state is only saved persistently when "OK" is
  // pressed. We need to manually restore the current selection
  // when the widget is refreshed.
  std::set<int> indices = get();
  monitors.clear();

  // FLTK recursively deletes all children for us.
  clear();
  begin();
  layout();
  end();

  // Restore the current selection state.
  set(indices);
  redraw();
}

bool MonitorArrangement::is_required(int m)
{
  // A selected monitor is never required.
  if (monitors[m]->value() == 1)
    return false;

  // If no monitors are selected, none are required.
  std::set<int> selected = get();
  if (selected.size() <= 0)
    return false;


  // Go through all selected monitors and find the monitor
  // indices that bounds the fullscreen frame buffer. If
  // the given monitor's coordinates are inside the bounds,
  // while not being selected, it is instead required.

  int x, y, w, h;
  int top_y, bottom_y, left_x, right_x;
  std::set<int>::iterator it = selected.begin();

  // Base the rest of the calculations on the dimensions
  // obtained for the first monitor.
  Fl::screen_xywh(x, y, w, h, *it);
  top_y = y;
  bottom_y = y + h;
  left_x = x;
  right_x = x + w;

  // Go through the rest of the monitors,
  // exhausting the rest of the iterator.
  for (; it != selected.end(); it++) {
    Fl::screen_xywh(x, y, w, h, *it);

    if (y < top_y) {
      top_y = y;
    }

    if ((y + h) > bottom_y) {
      bottom_y = y + h;
    }

    if (x < left_x) {
      left_x = x;
    }

    if ((x + w) > right_x) {
      right_x = x + w;
    }
  }


  Fl::screen_xywh(x, y, w, h, m);

  if (x < left_x)
    return false;
  if ((x + w) > right_x)
    return false;
  if (y < top_y)
    return false;
  if ((y + h) > bottom_y)
    return false;

  return true;
}

double MonitorArrangement::scale()
{
  const int MARGIN = 20;
  std::pair<int, int> size = this->size();

  double s_w = static_cast<double>(this->w()-MARGIN) / static_cast<double>(size.first);
  double s_h = static_cast<double>(this->h()-MARGIN) / static_cast<double>(size.second);

  // Choose the one that scales the least, in order to
  // maximize our use of the given bounding area.
  if (s_w > s_h)
    return s_h;
  else
    return s_w;
}

std::pair<int, int> MonitorArrangement::size()
{
  int x, y, w, h;
  int top, bottom, left, right;
  int x_min, x_max, y_min, y_max;
  x_min = x_max = y_min = y_max = 0;

  for (int i = 0; i < Fl::screen_count(); i++) {
    Fl::screen_xywh(x, y, w, h, i);

    top = y;
    bottom = y + h;
    left = x;
    right = x + w;

    if (top < y_min)
      y_min = top;

    if (bottom > y_max)
      y_max = bottom;

    if (left < x_min)
      x_min = left;

    if (right > x_max)
      x_max = right;
  }

  return std::make_pair(x_max - x_min, y_max - y_min);
}

std::pair<int, int> MonitorArrangement::offset()
{
  double scale = this->scale();
  std::pair<int, int> size = this->size();
  std::pair<int, int> origin = this->origin();

  int offset_x = (this->w()/2) - (size.first/2 * scale);
  int offset_y = (this->h()/2) - (size.second/2 * scale);

  return std::make_pair(offset_x + abs(origin.first)*scale, offset_y + abs(origin.second)*scale);
}

std::pair<int, int> MonitorArrangement::origin()
{
  int x, y, w, h, ox, oy;
  ox = oy = 0;

  for (int i = 0; i < Fl::screen_count(); i++) {
    Fl::screen_xywh(x, y, w, h, i);

    if (x < ox)
      ox = x;

    if (y < oy)
      oy = y;
  }

  return std::make_pair(ox, oy);
}

std::string MonitorArrangement::description(int m)
{
  std::string name;
  int x, y, w, h;
  std::stringstream ss;

  assert(m < Fl::screen_count());

  name = get_monitor_name(m);
  Fl::screen_xywh(x, y, w, h, m);

  if (!name.empty())
    ss << name << " (" << w << "x" << h << ")";
  else
    ss << w << "x" << h;

  return ss.str();
}

#if defined(WIN32)
static BOOL CALLBACK EnumDisplayMonitorsCallback(
  HMONITOR monitor, HDC deviceContext, LPRECT rect, LPARAM userData)
{
  std::set<HMONITOR>* sys_monitors = (std::set<HMONITOR>*)userData;
  sys_monitors->insert(monitor);
  return TRUE;
}
#endif

std::string MonitorArrangement::get_monitor_name(int m)
{
#if defined(WIN32)
  std::set<HMONITOR> sys_monitors;
  std::set<HMONITOR>::const_iterator iter;
  int x, y, w, h;

  Fl::screen_xywh(x, y, w, h, m);

  EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsCallback,
                      (LPARAM)&sys_monitors);

  for (iter = sys_monitors.begin(); iter != sys_monitors.end(); ++iter) {
    MONITORINFOEX info;
    DISPLAY_DEVICE dev;
    std::string name;

    info.cbSize = sizeof(info);
    GetMonitorInfo(*iter, (LPMONITORINFO)&info);

    if (info.rcMonitor.left != x)
      continue;
    if (info.rcMonitor.top != y)
      continue;
    if ((info.rcMonitor.right - info.rcMonitor.left) != w)
      continue;
    if ((info.rcMonitor.bottom - info.rcMonitor.top) != h)
      continue;

    for (int i = 0; ; i++) {
      dev.cb = sizeof(dev);
      if (!EnumDisplayDevices(info.szDevice, i, &dev, 0))
        break;

      if (!(dev.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        continue;

      if (!name.empty())
        name += " / ";
      name += dev.DeviceString;
    }

    return name;
  }

  return "";
#elif defined(__APPLE__)
  CGDisplayCount count;
  CGDirectDisplayID displays[16];

  CGDirectDisplayID displayID;
  CFDictionaryRef info;
  CFDictionaryRef dict;
  CFIndex dict_len;

  std::string name;

  if (CGGetActiveDisplayList(16, displays, &count) != kCGErrorSuccess)
    return "";

  if (count != (unsigned)Fl::screen_count())
    return "";

  if (m >= (int)count)
    return "";

  // Notice: Here we assume indices to be ordered the same as in FLTK (we rely on that in cocoa.mm as well).
  displayID = displays[m];

  info = IODisplayCreateInfoDictionary(CGDisplayIOServicePort(displayID),
                                       kIODisplayOnlyPreferredName);
  if (info == NULL)
    return "";

  dict = (CFDictionaryRef) CFDictionaryGetValue(info, CFSTR(kDisplayProductName));
  if (dict == NULL) {
    CFRelease(info);
    return "";
  }

  dict_len = CFDictionaryGetCount(dict);

  if (dict_len > 0) {
    CFTypeRef * names = new CFTypeRef[dict_len];
    CFDictionaryGetKeysAndValues(dict, NULL, (const void **) names);

    if (names[0]) {

      // Because of `kIODisplayOnlyPreferredName` names *should* only contain the name with
      // the current system localization.
      CFStringRef localized_name = (CFStringRef) names[0];
      CFIndex localized_name_len = CFStringGetLength(localized_name);

      // Even though we already have the length of `localized_name` above, we know that we will format it
      // as UTF-8 when we put it in the destination buffer. Therefore we need to check whether the name
      // with that encoding will fit.
      CFIndex localized_name_max_size = CFStringGetMaximumSizeForEncoding(localized_name_len, kCFStringEncodingUTF8) + 1;

      if (localized_name_max_size != kCFNotFound) {
        char *utf8_name = new char[localized_name_max_size];
        if (CFStringGetCString(
          /* ref = */ localized_name,
          /* dest = */ utf8_name,
          /* dest_len = */ localized_name_max_size,
          /* encoding = */ kCFStringEncodingUTF8))
        {
          name = utf8_name;
        }
        delete [] utf8_name;
      }
    }

    delete[] names;
  }

  CFRelease(info);

  return name;

#else
#if defined (HAVE_XRANDR)
  int x, y, w, h;
  int ev, err, xi_major;
  std::string name;

  fl_open_display();
  assert(fl_display != NULL);
  Fl::screen_xywh(x, y, w, h, m);

  if (!XQueryExtension(fl_display, "RANDR", &xi_major, &ev, &err))
    return "";

  XRRScreenResources *res = XRRGetScreenResources(fl_display, DefaultRootWindow(fl_display));
  if (!res)
    return "";

  for (int i = 0; i < res->ncrtc; i++) {
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(fl_display, res, res->crtcs[i]);

    if (!crtc)
      continue;

    if ((crtc->x != x) || (crtc->y != y) ||
        ((int)crtc->width != w) || ((int)crtc->height != h))
      continue;

    for (int j = 0; j < crtc->noutput; j++) {
      XRROutputInfo *output;

      output = XRRGetOutputInfo(fl_display, res, crtc->outputs[j]);
      if (!output)
        continue;

      if (!name.empty())
        name += " / ";
      name += output->name;
    }
  }

  return name;

#endif // !HAVE_XRANDR
  return "";
#endif
}

int MonitorArrangement::fltk_event_handler(int event)
{
  std::set<MonitorArrangement *>::iterator it;

  if (event != FL_SCREEN_CONFIGURATION_CHANGED)
    return 0;

  for (it = instances.begin(); it != instances.end(); it++)
    (*it)->refresh();

  return 0;
}

void MonitorArrangement::monitor_pressed(Fl_Widget *widget, void *user_data)
{
  MonitorArrangement *self = (MonitorArrangement *) user_data;

  // When a monitor is selected, FLTK changes the state of it for us.
  // However, selecting a monitor might implicitly change the state of
  // others (if they become required). FLTK only redraws the selected
  // monitor. Therefore, we must trigger a redraw of the whole widget
  // manually.
  self->redraw();
}

void MonitorArrangement::checkered_pattern_draw(
  int x, int y, int width, int height, Fl_Color color)
{
  bool draw_checker = false;
  const int CHECKER_SIZE = 8;

  fl_color(fl_lighter(fl_lighter(fl_lighter(color))));
  fl_rectf(x, y, width, height);

  fl_color(Fl::draw_box_active() ? color : fl_inactive(color));

  // Round up the square count. Later on, we remove square area that are
  // outside the given bounding area.
  const int count = (width + CHECKER_SIZE - 1) / CHECKER_SIZE;

  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {

      draw_checker = (i + j) % 2 == 0;

      if (draw_checker) {
        fl_rectf(
          /* x = */ x + i * CHECKER_SIZE,
          /* y = */ y + j * CHECKER_SIZE,
          /* w = */ CHECKER_SIZE - std::max(0, ((i + 1) * CHECKER_SIZE) - width),
          /* h = */ CHECKER_SIZE - std::max(0, ((j + 1) * CHECKER_SIZE) - height)
        );
      }
    }
  }

  fl_color(Fl::draw_box_active() ? FL_BLACK : fl_inactive(FL_BLACK));
  fl_rect(x, y, width, height);
}
