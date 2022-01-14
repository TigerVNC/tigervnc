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

#include "theme.h"

void init_theme()
{
  // Basic text size (10pt @ 96 dpi => 13px)
  // FIXME: This is small by modern standards, but:
  //        https://github.com/fltk/fltk/issues/371
  FL_NORMAL_SIZE = 13;

  // Select a FLTK scheme and background color that looks somewhat
  // close to modern systems
  Fl::scheme("gtk+");

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

  // This makes the "icon" in dialogs rounded, which fits better
  // with the above schemes.
  fl_message_icon()->box(FL_UP_BOX);

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
