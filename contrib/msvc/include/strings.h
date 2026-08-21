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

#ifndef __MSVC_STRINGS_H__
#define __MSVC_STRINGS_H__

#include <intrin.h>
#include <string.h>

#define strcasecmp _stricmp
#define strncasecmp _strnicmp

// One plus the index of the least significant set bit, or zero if there
// is none.
static inline int ffs(int i)
{
  unsigned long bit;

  if (!_BitScanForward(&bit, (unsigned long)i))
    return 0;

  return (int)bit + 1;
}

#endif
