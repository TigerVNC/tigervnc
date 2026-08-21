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

#ifndef __MSVC_LIBGEN_H__
#define __MSVC_LIBGEN_H__

#include <string.h>

// Strips the trailing component, in place, which POSIX permits. Both
// separators are accepted as Windows paths use either.
static inline char* dirname(char* path)
{
  static char dot[] = ".";
  char* slash;
  char* fwdslash;

  slash = strrchr(path, '\\');
  fwdslash = strrchr(path, '/');

  if (fwdslash > slash)
    slash = fwdslash;

  if (slash == NULL)
    return dot;

  if (slash == path) {
    *(slash + 1) = '\0';
    return path;
  }

  *slash = '\0';

  return path;
}

#endif
