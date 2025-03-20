/* Copyright 2011-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __VNCVIEWER_COCOA_H__
#define __VNCVIEWER_COCOA_H__

class Fl_Window;

void cocoa_prevent_native_fullscreen(Fl_Window *win);

bool cocoa_is_trusted(bool prompt=false);

bool cocoa_tap_keyboard();
void cocoa_untap_keyboard();

typedef struct CGColorSpace *CGColorSpaceRef;

CGColorSpaceRef cocoa_win_color_space(Fl_Window *win);

bool cocoa_win_is_zoomed(Fl_Window *win);
void cocoa_win_zoom(Fl_Window *win);

#endif
