/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

#import <Cocoa/Cocoa.h>

static bool captured = false;

int cocoa_capture_display(Fl_Window *win, bool all_displays)
{
  NSWindow *nsw;

  nsw = (NSWindow*)fl_xid(win);

  if (!captured) {
    if (all_displays) {
      if (CGCaptureAllDisplays() != kCGErrorSuccess)
        return 1;
    } else {
      CGDirectDisplayID displays[16];
      CGDisplayCount count;
      int index;

      if (CGGetActiveDisplayList(16, displays, &count) != kCGErrorSuccess)
        return 1;

      if (count != Fl::screen_count())
        return 1;

#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
      index = Fl::screen_num(win->x(), win->y(), win->w(), win->h());
#else
      index = 0;
#endif

      if (CGDisplayCapture(displays[index]) != kCGErrorSuccess)
        return 1;
    }

    captured = true;
  }

  if ([nsw level] == CGShieldingWindowLevel())
    return 0;

  [nsw setLevel:CGShieldingWindowLevel()];

  return 0;
}

void cocoa_release_display(Fl_Window *win)
{
  NSWindow *nsw;
  int newlevel;

  if (captured)
    CGReleaseAllDisplays();

  captured = false;

  nsw = (NSWindow*)fl_xid(win);

  // Someone else has already changed the level of this window
  if ([nsw level] != CGShieldingWindowLevel())
    return;

  // FIXME: Store the previous level somewhere so we don't have to hard
  //        code a level here.
#ifdef HAVE_FLTK_FULLSCREEN
  if (win->fullscreen_active() && win->contains(Fl::focus()))
    newlevel = NSStatusWindowLevel;
  else
#endif
    newlevel = NSNormalWindowLevel;

  // Only change if different as the level change also moves the window
  // to the top of that level.
  if ([nsw level] != newlevel)
    [nsw setLevel:newlevel];
}
