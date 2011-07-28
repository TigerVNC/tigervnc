//
// "$Id: Fl_Window_fullscreen.cxx 8515 2011-03-12 21:36:21Z manolo $"
//
// Fullscreen window support for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// Turning the border on/off by changing the motif_wm_hints property
// works on Irix 4DWM.  Does not appear to work for any other window
// manager.  Fullscreen still works on some window managers (fvwm is one)
// because they allow the border to be placed off-screen.

// Unfortunately most X window managers ignore changes to the border
// and refuse to position the border off-screen, so attempting to make
// the window full screen will lose the size of the border off the
// bottom and right.

#include <FL/Fl.H>
#include <FL/x.H>

#include <config.h>

void Fl_Window::border(int b) {
  if (b) {
    if (border()) return;
    clear_flag(NOBORDER);
  } else {
    if (!border()) return;
    set_flag(NOBORDER);
  }
#if defined(USE_X11)
  if (shown()) Fl_X::i(this)->sendxjunk();
#elif defined(WIN32)
  // not yet implemented, but it's possible
  // for full fullscreen we have to make the window topmost as well
#elif defined(__APPLE_QUARTZ__)
  // warning: not implemented in Quartz/Carbon
#else
# error unsupported platform
#endif
}

void fullscreen_x(Fl_Window *w);
void fullscreen_off_x();
void fullscreen_off_x(Fl_Window *w, int X, int Y, int W, int H);

/* Note: The previous implementation toggled border(). With this new
   implementation this is not necessary. Additionally, if we do that,
   the application may lose focus when switching out of fullscreen
   mode with some window managers. Besides, the API does not say that
   the FLTK border state should be toggled; it only says that the
   borders should not be *visible*. 
*/
void Fl_Window::fullscreen() {
  if (shown() && !(flags() & Fl_Widget::FULLSCREEN)) {
    no_fullscreen_x = x();
    no_fullscreen_y = y();
    no_fullscreen_w = w();
    no_fullscreen_h = h();
    fullscreen_x(this);
  } else {
    set_flag(FULLSCREEN);
  }
}

void Fl_Window::fullscreen_off(int X,int Y,int W,int H) {
  if (shown() && (flags() & Fl_Widget::FULLSCREEN)) {
    fullscreen_off_x(this, X, Y, W, H);
  } else {
    clear_flag(FULLSCREEN);
  }
  no_fullscreen_x = no_fullscreen_y = no_fullscreen_w = no_fullscreen_h = 0;
}

void Fl_Window::fullscreen_off() {
  if (!no_fullscreen_x && !no_fullscreen_y) {
    // Window was initially created fullscreen - default to current monitor
    no_fullscreen_x = x();
    no_fullscreen_y = y();
  }
  fullscreen_off(no_fullscreen_x, no_fullscreen_y, no_fullscreen_w, no_fullscreen_h);
}


//
// End of "$Id: Fl_Window_fullscreen.cxx 8515 2011-03-12 21:36:21Z manolo $".
//
