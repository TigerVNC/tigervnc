/* Copyright 2026 jose-pr
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

#ifndef __MSVC_COMPAT_H__
#define __MSVC_COMPAT_H__

// Force included before anything else, so that the definitions below are
// in place by the time any header or source line needs them.

// The sources decorate printf style functions with GCC's format
// attribute. MSVC has no equivalent, and no way to ignore an attribute
// it does not know, so they compile away to nothing.
#define __attribute__(x)

// <math.h> only defines M_PI and friends when asked to.
#define _USE_MATH_DEFINES

// The sources use the alternative spellings of the logical operators,
// which MSVC only recognises through this header.
#include <iso646.h>

// <windows.h> defines min() and max() as macros, which breaks both
// std::min() and any member function of that name.
#define NOMINMAX

// <winsock2.h> must come before <windows.h>, and something has to
// guarantee that when both are reachable from the same source.
#include <winsock2.h>

#include <basetsd.h>
#include <stdlib.h>

// POSIX spells this in <limits.h>; MSVC has its own name in <stdlib.h>.
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

// POSIX types that glibc declares from headers the sources already
// include, and that MSVC either spells differently or lacks.
typedef int mode_t;
typedef SSIZE_T ssize_t;

// glibc's <string.h> pulls in <strings.h>; MSVC's does not, and the
// sources rely on that.
#include <strings.h>

// GCC offers this as a builtin as well as a libc function.
#define __builtin_ffs ffs

#include <direct.h>

// POSIX mkdir() takes a mode; Windows has no equivalent permissions to
// apply it to.
static inline int mkdir(const char* path, mode_t mode)
{
  (void)mode;
  return _mkdir(path);
}

#endif
