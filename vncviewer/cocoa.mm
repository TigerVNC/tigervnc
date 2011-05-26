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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

#import <Cocoa/Cocoa.h>

int cocoa_capture_display(Fl_Window *win)
{
  NSWindow *nsw;

  nsw = (NSWindow*)fl_xid(win);

  // Already captured?
  if ([nsw level] == CGShieldingWindowLevel())
    return 0;

  if (CGDisplayCapture(kCGDirectMainDisplay) != kCGErrorSuccess)
    return 1;

  [nsw setLevel:CGShieldingWindowLevel()];

  return 0;
}

void cocoa_release_display(Fl_Window *win)
{
  NSWindow *nsw;
  int newlevel;

  CGDisplayRelease(kCGDirectMainDisplay);

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
