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

#ifdef HAVE_COMMON_CONFIG_H
#include <common-config.h>
#endif

#include <os/print.h>
#include <stdarg.h>

#ifndef HAVE_VSNPRINTF
int vsnprintf(char *str, size_t n, const char *format, va_list ap) {
	static FILE *fp = NULL;
	va_list ap_new;
	int len, written;

	if (n < 1)
		return 0;

	str[0] = '\0';
	if (fp == NULL) {
		fp = fopen("/dev/null","w");
		if (fp == NULL)
			return 0;
	}

	va_copy(ap_new, ap);
	len = vfprintf(fp, format, ap_new);
	va_end(ap_new);

	if (len <= 0)
		return 0;

	CharArray s(len+1);
	vsprintf(s.buf, format, ap);

	written = (len < (n - 1)) ? len : (n - 1);
	memcpy(str, s.buf, written);
	str[written] = '\0';
	return len;
}
#endif /* HAVE_VSNPRINTF */

