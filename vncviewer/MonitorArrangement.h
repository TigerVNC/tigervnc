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

#ifndef __MONITOR_ARRANGEMENT_H__
#define __MONITOR_ARRANGEMENT_H__

#include <string>
#include <map>
#include <set>

#include <FL/Fl_Group.H>

class Fl_Button;

class MonitorArrangement: public Fl_Group {
public:
  MonitorArrangement(int x, int y, int w, int h);
  ~MonitorArrangement();

  // Get selected indices.
  std::set<int> get();

  // Set selected indices.
  void set(std::set<int> indices);

protected:
  virtual void draw();

private:
  const Fl_Color SELECTION_COLOR;
  const Fl_Color AVAILABLE_COLOR;
  typedef std::map<int, Fl_Button *> MonitorMap;
  MonitorMap monitors;

  // Layout the monitor arrangement.
  void layout();

  // Refresh the monitor arrangement.
  void refresh();

  // Return true if the given monitor is required to be part of the configuration
  // for it to be valid. A configuration is only valid if the framebuffer created
  // from is rectangular.
  bool is_required(int m);

  // Calculate the scale of the monitor arrangement.
  double scale();

  // Return the size of the monitor arrangement.
  std::pair<int, int> size();

  // Return the offset required for centering the monitor
  // arrangement in the given bounding area.
  std::pair<int, int> offset();

  // Return the origin of the monitor arrangement (top left corner).
  std::pair<int, int> origin();

  // Get a textual description of the given monitor.
  std::string description(int m);
  std::string get_monitor_name(int m);

  static int fltk_event_handler(int event);
  static void monitor_pressed(Fl_Widget *widget, void *user_data);
  static void checkered_pattern_draw(
    int x, int y, int width, int height, Fl_Color color);
};

#endif
