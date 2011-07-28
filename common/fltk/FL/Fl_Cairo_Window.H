//
// "$Id: Fl_Cairo_Window.H 8198 2011-01-06 10:24:58Z manolo $"
//
// Main header file for the Fast Light Tool Kit (FLTK).
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

/* \file
    Fl_Cairo_Window Handling transparently a fltk window incorporte a cairo draw callback.
*/

#ifndef FL_CAIRO_WINDOW_H
# define FL_CAIRO_WINDOW_H
# ifdef FLTK_HAVE_CAIRO

// Cairo is currently supported for the following platforms:
// Win32, Apple Quartz, X11
#  include <FL/Fl.H>
#  include <FL/Fl_Double_Window.H>

/** 
   \addtogroup group_cairo
   @{
*/

/**
   This defines a pre-configured cairo fltk window.
   This class overloads the virtual draw() method for you,
   so that the only thing you have to do is to provide your cairo code.
   All cairo context handling is achieved transparently.
   \note You can alternatively define your custom cairo fltk window,
   and thus at least override the draw() method to provide custom cairo
   support. In this case you will probably use Fl::cairo_make_current(Fl_Window*)
   to attach a context to your window. You should do it only when your window is 
   the current window. \see Fl_Window::current()
*/
class FL_EXPORT Fl_Cairo_Window : public Fl_Double_Window {

public:
  Fl_Cairo_Window(int w, int h) : Fl_Double_Window(w,h),draw_cb_(0) {}

protected:
  /** Overloaded to provide cairo callback support */
  void draw() {
    Fl_Double_Window::draw();
    // manual method ? if yes explicitly get a cairo_context here
    if (!Fl::cairo_autolink_context()) 
      Fl::cairo_make_current(this); 
    if (draw_cb_) draw_cb_(this, Fl::cairo_cc());
  }

public:
  /** This defines the cairo draw callback prototype that you must further */
  typedef void (*cairo_draw_cb) (Fl_Cairo_Window* self, cairo_t* def);
  /** 
    You must provide a draw callback which will implement your cairo rendering.
    This method will permit you to set your cairo callback to \p cb.
  */
  void set_draw_cb(cairo_draw_cb  cb){draw_cb_=cb;}
private:
    cairo_draw_cb draw_cb_;
};


/** @} */

# endif // FLTK_HAVE_CAIRO
#endif // FL_CAIRO_WINDOW_H

//
// End of "$Id: Fl_Cairo_Window.H 8198 2011-01-06 10:24:58Z manolo $" .
//
