/* Copyright 2021 Hugo Lundin <huglu@cendio.se> for Cendio AB.
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

#include <rfb/Rect.h>

#include "MonitorArrangement.h"

static const Fl_Boxtype FL_CHECKERED_BOX = FL_FREE_BOXTYPE;

MonitorArrangement::MonitorArrangement(
   int x, int y, int w, int h)
:  Fl_Group(x, y, w, h),
   SELECTION_COLOR(fl_lighter(FL_BLUE)),
   AVAILABLE_COLOR(fl_lighter(fl_lighter(fl_lighter(FL_BACKGROUND_COLOR)))),
   m_monitors()
{
  // Used for required monitors.
  Fl::set_boxtype(FL_CHECKERED_BOX, checkered_pattern_draw, 0, 0, 0, 0);

  box(FL_DOWN_BOX);
  color(fl_lighter(FL_BACKGROUND_COLOR));
  layout();
  end();
}

MonitorArrangement::~MonitorArrangement()
{

}

std::set<int> MonitorArrangement::get()
{
  std::set<int> indices;

  for (int i = 0; i < (int) m_monitors.size(); i++) {
    if (m_monitors[i]->value() == 1)
      indices.insert(i);
  }

  return indices;
}

void MonitorArrangement::set(std::set<int> indices)
{
  for (int i = 0; i < (int) m_monitors.size(); i++) {
    bool selected = std::find(indices.begin(), indices.end(), i) != indices.end();
    m_monitors[i]->value(selected ? 1 : 0);
  }
}

void MonitorArrangement::draw()
{
  for (int i = 0; i < (int) m_monitors.size(); i++) {
    Fl_Button * monitor = m_monitors[i];

    if (is_required(i)) {
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
    Fl::screen_xywh(x, y, w, h, i);

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
    m_monitors.push_back(monitor);
  }
}

bool MonitorArrangement::is_required(int m)
{
  // A selected monitor is never required.
  if (m_monitors[m]->value() == 1)
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

  rfb::Rect viewport, monitor;
  viewport.setXYWH(left_x, top_y, right_x - left_x, bottom_y - top_y);

  Fl::screen_xywh(x, y, w, h, m);
  monitor.setXYWH(x, y, w, h);

  return monitor.enclosed_by(viewport);
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
