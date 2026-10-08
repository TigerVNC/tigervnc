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

#define __attribute__(x)
#define __builtin_ffs ffs

#define _USE_MATH_DEFINES
#define NOMINMAX

#include <iso646.h>
#include <winsock2.h>

#include <basetsd.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

typedef int mode_t;
typedef SSIZE_T ssize_t;

#include <strings.h>
#include <direct.h>

static inline int mkdir(const char* path, mode_t mode)
{
  (void)mode;
  return _mkdir(path);
}

#endif
