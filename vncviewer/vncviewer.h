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

#ifndef __VNCVIEWER_H__
#define __VNCVIEWER_H__

#define VNCSERVERNAMELEN 256

#ifdef __GNUC__
#  define __printf_attr(a, b) __attribute__((__format__ (__printf__, a, b)))
#else
#  define __printf_attr(a, b)
#endif // __GNUC__

void exit_vncviewer(const char *error = NULL, ...) __printf_attr(1, 2);
bool should_exit();
void about_vncviewer();
void run_mainloop();

#endif
