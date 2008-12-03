/* Copyright (C) 2008 TightVNC Team.  All Rights Reserved.
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

#ifndef OS_PRINT_H
#define OS_PRINT_H

#ifdef HAVE_COMMON_CONFIG_H
#include <common-config.h>
#endif

#ifdef WIN32
#include <common-config.win.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_VSNPRINTF
/* NOTE:
 *
 * This is only very limited implementation for our internal purposes. It
 * doesn't conform to C99/POSIX
 * - limited conversion specifiers
 * - returns written number of characters instead of number what would be
 *   written
 */
int tight_vsnprintf(char *str, size_t n, const char *format, va_list ap);
#define vsnprintf tight_vsnprintf
#endif

#ifndef HAVE_SNPRINTF
/* Inherits tight_vsnprintf limitations if vsnprintf is not present */
int tight_snprintf(char *str, size_t n, const char *format, ...);
#define snprintf tight_snprintf
#endif

#ifdef __cplusplus
};
#endif

#endif /* OS_PRINT_H */
