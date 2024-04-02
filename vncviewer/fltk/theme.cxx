/* Copyright 2011-2022 Pierre Ossman for Cendio AB
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

#include <assert.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include "theme.h"

const int RADIUS = 4;

/*
 * fl_arc() and fl_pie() are broken on Windows, so we need to fudge the
 * numbers a bit
 */
#ifdef WIN32
static void fixed_arc(int x,int y,int w,int h,double a1,double a2) {
  if ((a1 != 0.0) || (a2 != 360.0)) {
    a1 -= 10.0;
    a2 += 10.0;
  }
  fl_arc(x, y, w, h, a1, a2);
}

static void fixed_pie(int x,int y,int w,int h,double a1,double a2) {
  if ((a1 != 0.0) || (a2 != 360.0)) {
    a1 -= 10.0;
    a2 += 10.0;
  }
  fl_pie(x, y, w, h, a1, a2);
}

#define fl_arc fixed_arc
#define fl_pie fixed_pie
#endif

static Fl_Color light_border(Fl_Color c)
{
  return fl_color_average(FL_BLACK, c, 0.12);
}

static Fl_Color dark_border(Fl_Color c)
{
  return fl_color_average(FL_BLACK, c, 0.33);
}

static void theme_round_frame(bool up, int x, int y, int w, int h,
                              int r, Fl_Color c)
{
  if (r > h/2)
    r = h/2;

  // Top
  if (up)
    Fl::set_box_color(light_border(c));
  else
    Fl::set_box_color(dark_border(c));

  fl_xyline(x+r, y, x+w-r-1);

  // Top corners
  if (!up)
    Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));

  fl_arc(x, y, r*2, r*2, 90.0, 180.0);
  fl_arc(x+w-(r*2), y, r*2, r*2, 0, 90.0);

  // Left and right
  Fl::set_box_color(light_border(c));

  fl_yxline(x, y+r, y+h-r-1);
  fl_yxline(x+w-1, y+r, y+h-r-1);

  // Bottom corners
  if (up)
    Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));

  fl_arc(x, y+h-(r*2), r*2, r*2, 180, 270.0);
  fl_arc(x+w-(r*2), y+h-(r*2), r*2, r*2, 270, 360.0);

  // Bottom
  if (up)
    Fl::set_box_color(dark_border(c));
  else
    Fl::set_box_color(light_border(c));

  fl_xyline(x+r, y+h-1, x+w-r-1);
}

static void theme_up_frame(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_frame(true, x, y, w, h, RADIUS, c);
}

static void theme_down_frame(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_frame(false, x, y, w, h, RADIUS, c);
}

static void theme_round_rect(int x, int y, int w, int h, int r,
                             Fl_Color c, Fl_Color c2=-1)
{
  if (r > h/2)
   r = h/2;

  if (c2 == (Fl_Color)-1)
    c2 = c;

  // Top
  Fl::set_box_color(c);

  fl_pie(x, y, r*2, r*2, 90.0, 180.0);
  fl_rectf(x+r, y, w-(r*2), r);
  fl_pie(x+w-(r*2), y, r*2, r*2, 0, 90.0);

  // Middle
  const int steps = h - r*2;
  for (int i = 0; i < steps; i++) {
    Fl::set_box_color(fl_color_average(c2, c, (double)i/steps));
    fl_xyline(x, y+r+i, x+w-1);
  }

  // Bottom
  Fl::set_box_color(c2);

  fl_pie(x, y+h-(r*2), r*2, r*2, 180, 270.0);
  fl_rectf(x+r, y+h-r, w-(r*2), r);
  fl_pie(x+w-(r*2), y+h-(r*2), r*2, r*2, 270, 360.0);
}

static void theme_up_box(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_rect(x, y, w, h, RADIUS,
                   fl_color_average(FL_WHITE, c, 0.75));
  theme_up_frame(x, y, w, h, c);
}

static void theme_down_box(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_rect(x, y, w, h, RADIUS, c);
  theme_down_frame(x, y, w, h, c);
}

static void theme_thin_up_box(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_rect(x, y, w, h, RADIUS, c);
  theme_up_frame(x, y, w, h, c);
}

static void theme_thin_down_box(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_rect(x, y, w, h, RADIUS, c);
  theme_down_frame(x, y, w, h, c);
}

static void theme_round_up_box(int x, int y, int w, int h, Fl_Color c)
{
  // Background
  Fl::set_box_color(c);
  fl_pie(x, y, w, h, 0.0, 360.0);

  // Outline
  Fl::set_box_color(light_border(c));
  fl_arc(x, y, w, h, 0.0, 180.0);
  Fl::set_box_color(dark_border(c));
  fl_arc(x, y, w, h, 180.0, 360.0);
  Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));
  fl_arc(x, y, w, h, 225.0, 315.0);
}

static void theme_round_down_box(int x, int y, int w, int h, Fl_Color c)
{
  // Background
  Fl::set_box_color(c);
  fl_pie(x, y, w, h, 0.0, 360.0);

  // Outline
  Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));
  fl_arc(x, y, w, h, 0.0, 180.0);
  Fl::set_box_color(dark_border(c));
  fl_arc(x, y, w, h, 45.0, 135.0);
  Fl::set_box_color(light_border(c));
  fl_arc(x, y, w, h, 180.0, 360.0);
}

void init_theme()
{
#if defined(WIN32) || defined(__APPLE__)
  static char font_name[256];
#endif

  // Basic text size (10pt @ 96 dpi => 13px)
  FL_NORMAL_SIZE = 13;

  // Modern colors based on the default appearance of Windows 11,
  // macOS 12 and GNOME 41

  // FIXME: Should get these from the system,
  //        Fl::get_system_colors() is unfortunately not very capable
  // FIXME: Should also handle dark mode

#if defined(WIN32)
  // Windows 11
  Fl::foreground(26, 26, 26);
  Fl::background(243, 243, 243);
#elif defined(__APPLE__)
  // FIXME: Text is rendered slightly lighter than what we specify here
  //        for some odd reason. The target is (38, 38, 38).
  Fl::foreground(28, 28, 28);
  Fl::background(246, 246, 246);
#else
  // GNOME
  Fl::foreground(46, 52, 54);
  Fl::background(246, 245, 244);
#endif

#if defined(WIN32)
  // Windows 11 default accent color
  Fl::set_color(FL_SELECTION_COLOR, 0, 103, 192);
#elif defined(__APPLE__)
  Fl::set_color(FL_SELECTION_COLOR, 0, 122, 255);
#else
  // GNOME
  Fl::set_color(FL_SELECTION_COLOR, 53, 132, 228);
#endif

  // The arrow on Fl_Return_Button gets a invisible, so let's adjust it
  // to compensate for our lighter buttons
  Fl::set_color(FL_LIGHT3, light_border(fl_color_average(FL_WHITE, FL_BACKGROUND_COLOR, 0.5)));
  Fl::set_color(FL_DARK3, dark_border(fl_color_average(FL_WHITE, FL_BACKGROUND_COLOR, 0.5)));

  // We will override the box types later, but changing scheme affects
  // more things than just those, so we still want to switch from the
  // default
  Fl::scheme("gtk+");

  // Define our box types
  const int PX = 2;
  const int PY = 2;

  // FLTK lacks a bounds check
  assert(THEME_ROUND_DOWN_BOX < 256);

  Fl::set_boxtype(THEME_UP_FRAME, theme_up_frame, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_DOWN_FRAME, theme_down_frame, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_THIN_UP_FRAME, theme_up_frame, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_THIN_DOWN_FRAME, theme_down_frame, PX, PY, PX*2, PY*2);

  Fl::set_boxtype(THEME_UP_BOX, theme_up_box, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_DOWN_BOX, theme_down_box, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_THIN_UP_BOX, theme_thin_up_box, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_THIN_DOWN_BOX, theme_thin_down_box, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_ROUND_UP_BOX, theme_round_up_box, PX, PY, PX*2, PY*2);
  Fl::set_boxtype(THEME_ROUND_DOWN_BOX, theme_round_down_box, PX, PY, PX*2, PY*2);

  // Make them the default
  Fl::set_boxtype(FL_UP_FRAME,        THEME_UP_FRAME);
  Fl::set_boxtype(FL_DOWN_FRAME,      THEME_UP_FRAME);
  Fl::set_boxtype(FL_THIN_UP_FRAME,   THEME_THIN_UP_FRAME);
  Fl::set_boxtype(FL_THIN_DOWN_FRAME, THEME_THIN_DOWN_FRAME);

  Fl::set_boxtype(FL_UP_BOX,          THEME_UP_BOX);
  Fl::set_boxtype(FL_DOWN_BOX,        THEME_DOWN_BOX);
  Fl::set_boxtype(FL_THIN_UP_BOX,     THEME_THIN_UP_BOX);
  Fl::set_boxtype(FL_THIN_DOWN_BOX,   THEME_THIN_DOWN_BOX);
  Fl::set_boxtype(_FL_ROUND_UP_BOX,   THEME_ROUND_UP_BOX);
  Fl::set_boxtype(_FL_ROUND_DOWN_BOX, THEME_ROUND_DOWN_BOX);

#if defined(WIN32)
  NONCLIENTMETRICS metrics;
  metrics.cbSize = sizeof(metrics);
  if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                           sizeof(metrics), &metrics, 0)) {
    strcpy(font_name, metrics.lfMessageFont.lfFaceName);
    Fl::set_font(FL_HELVETICA, font_name);
  }
#elif defined(__APPLE__)
  CTFontRef font;
  CFStringRef name;

  font = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 0.0, nullptr);
  if (font != nullptr) {
    name = CTFontCopyFullName(font);
    if (name != nullptr) {
      CFStringGetCString(name, font_name, sizeof(font_name),
                         kCFStringEncodingUTF8);

      Fl::set_font(FL_HELVETICA, font_name);

      CFRelease(name);
    }
    CFRelease(font);
  }
#else
  // FIXME: Get font from GTK or QT
#endif
}
