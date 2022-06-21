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

// FIXME: Antialiased pie/arc

#ifdef WIN32
const int RADIUS = 4; // Windows 11
#else
const int RADIUS = 5; // GNOME / macOS
#endif

static Fl_Color shadow_color(Fl_Color c)
{
  return fl_color_average(FL_BLACK, c, 0.03);
}

static Fl_Color light_border(Fl_Color c)
{
#if defined(WIN32) || defined(__APPLE__)
  return fl_color_average(FL_BLACK, c, 0.12);
#else
  return fl_color_average(FL_BLACK, c, 0.17);
#endif
}

static Fl_Color dark_border(Fl_Color c)
{
#if defined(WIN32) || defined(__APPLE__)
  return fl_color_average(FL_BLACK, c, 0.33);
#else
  return light_border(c);
#endif
}

static void theme_frame(bool up, int x, int y, int w, int h, Fl_Color c)
{
  if (up)
    Fl::set_box_color(light_border(c));
  else
    Fl::set_box_color(dark_border(c));

  fl_xyline(x+RADIUS, y, x+w-RADIUS-1);

  if (!up)
    Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));

  fl_arc(x, y, RADIUS*2, RADIUS*2, 90.0, 180.0);
  fl_arc(x+w-(RADIUS*2), y, RADIUS*2, RADIUS*2, 0, 90.0);

  Fl::set_box_color(light_border(c));

  fl_yxline(x, y+RADIUS, y+h-RADIUS-1);
  fl_yxline(x+w-1, y+RADIUS, y+h-RADIUS-1);

  if (up)
    Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));

  fl_arc(x, y+h-(RADIUS*2), RADIUS*2, RADIUS*2, 180, 270.0);
  fl_arc(x+w-(RADIUS*2), y+h-(RADIUS*2), RADIUS*2, RADIUS*2, 270, 360.0);

  if (up)
    Fl::set_box_color(dark_border(c));
  else
    Fl::set_box_color(light_border(c));

  fl_xyline(x+RADIUS, y+h-1, x+w-RADIUS-1);
}

static void theme_up_frame(int x, int y, int w, int h, Fl_Color c)
{
  theme_frame(true, x, y, w, h, c);
}

static void theme_down_frame(int x, int y, int w, int h, Fl_Color c)
{
  theme_frame(false, x, y, w, h, c);
}

static void theme_round_rect(int x, int y, int w, int h, int r,
                             Fl_Color c, Fl_Color c2=-1)
{
  if (c2 == (Fl_Color)-1)
    c2 = c;

  Fl::set_box_color(c);

  fl_pie(x, y, r*2, r*2, 90.0, 180.0);
  fl_rectf(x+r, y, w-(r*2), r);
  fl_pie(x+w-(r*2), y, r*2, r*2, 0, 90.0);

  const int steps = h - r*2;
  for (int i = 0; i < steps; i++) {
    Fl::set_box_color(fl_color_average(c2, c, (double)i/steps));
    fl_xyline(x, y+r+i, x+w-1);
  }

  Fl::set_box_color(c2);

  fl_pie(x, y+h-(r*2), r*2, r*2, 180, 270.0);
  fl_rectf(x+r, y+h-r, w-(r*2), r);
  fl_pie(x+w-(r*2), y+h-(r*2), r*2, r*2, 270, 360.0);
}

static void theme_up_box(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_rect(x-1, y-1, w+2, h+2, RADIUS+1,
                   shadow_color(FL_BACKGROUND_COLOR));

#if defined(WIN32) || defined(__APPLE__)
  theme_round_rect(x, y, w, h, RADIUS, fl_lighter(fl_lighter(c)));
#else
  theme_round_rect(x, y, w, h, RADIUS,
                   c, fl_color_average(FL_BLACK, c, 0.04));
#endif

  theme_up_frame(x, y, w, h, c);
}

static void theme_down_box(int x, int y, int w, int h, Fl_Color c)
{
  theme_round_rect(x, y, w, h, RADIUS, shadow_color(c));

  theme_round_rect(x+2, y+2, w-4, h-4, RADIUS-2, c);

  theme_down_frame(x, y, w, h, c);
}

static void theme_round_up_box(int x, int y, int w, int h, Fl_Color c)
{
  Fl::set_box_color(shadow_color(FL_BACKGROUND_COLOR));
  fl_pie(x-1, y-1, w+2, h+2, 0.0, 360.0);

  Fl::set_box_color(c);
  fl_pie(x, y, w, h, 0.0, 360.0);

  Fl::set_box_color(light_border(c));
  fl_arc(x, y, w, h, 0.0, 180.0);
  Fl::set_box_color(dark_border(c));
  fl_arc(x, y, w, h, 180.0, 360.0);
  Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));
  fl_arc(x, y, w, h, 225.0, 315.0);
}

static void theme_round_down_box(int x, int y, int w, int h, Fl_Color c)
{
  Fl::set_box_color(shadow_color(c));
  fl_pie(x, y, w, h, 0.0, 360.0);

  Fl::set_box_color(c);
  fl_pie(x+2, y+2, w-4, h-4, 0.0, 360.0);

  Fl::set_box_color(fl_color_average(light_border(c), dark_border(c), 0.5));
  fl_arc(x, y, w, h, 0.0, 180.0);
  Fl::set_box_color(dark_border(c));
  fl_arc(x, y, w, h, 45.0, 135.0);
  Fl::set_box_color(light_border(c));
  fl_arc(x, y, w, h, 180.0, 360.0);
}

void init_theme()
{
  // Basic text size (11pt @ 96 dpi => 14.666...px)
  // FIXME: This is rounded down since there are some issues scaling
  //        fonts up in FLTK:
  //        https://github.com/fltk/fltk/issues/371
  // FIXME: macOS/Windows
  FL_NORMAL_SIZE = 14;

  // FIXME: Base theme
  Fl::scheme("gtk+");

  // Define our box types
  Fl::set_boxtype(THEME_UP_FRAME, theme_up_frame, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_DOWN_FRAME, theme_down_frame, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_THIN_UP_FRAME, theme_up_frame, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_THIN_DOWN_FRAME, theme_down_frame, 3, 3, 6, 6);

  Fl::set_boxtype(THEME_UP_BOX, theme_up_box, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_DOWN_BOX, theme_down_box, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_THIN_UP_BOX, theme_up_box, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_THIN_DOWN_BOX, theme_down_box, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_ROUND_UP_BOX, theme_round_up_box, 3, 3, 6, 6);
  Fl::set_boxtype(THEME_ROUND_DOWN_BOX, theme_round_down_box, 3, 3, 6, 6);

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

  // FIXME: Should get these from the system,
  //        Fl::get_system_colors() is unfortunately not very capable
  // FIXME: Should also handle dark mode
  // FIXME: fl_contrast() also sucks since its calculations are utterly
  //        wrong as they fail to account for sRGB curves. And even with
  //        that fixed we get odd results with dark backgrounds, so
  //        APCA should probably be used.
  //        https://github.com/fltk/fltk/issues/370
  Fl::foreground(46, 52, 54);
  Fl::background(246, 245, 244);
  // GNOME uses (53,132,228), macOS (0, 122, 255), this is the lightest
  // version of GNOME's that fl_contrast() won't screw up
  Fl::set_color(FL_SELECTION_COLOR, 29, 113, 215);

#if defined(WIN32)
  NONCLIENTMETRICS metrics;
  metrics.cbSize = sizeof(metrics);
  if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                           sizeof(metrics), &metrics, 0))
    Fl::set_font(FL_HELVETICA, metrics.lfMessageFont.lfFaceName);
#elif defined(__APPLE__)
  CTFontRef font;
  CFStringRef name;
  char cname[256];

  font = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 0.0, NULL);
  if (font != NULL) {
    name = CTFontCopyFullName(font);
    if (name != NULL) {
      CFStringGetCString(name, cname, sizeof(cname),
                         kCFStringEncodingUTF8);

      Fl::set_font(FL_HELVETICA, cname);

      CFRelease(name);
    }
    CFRelease(font);
  }
#else
  // FIXME: Get font from GTK or QT
#endif
}
