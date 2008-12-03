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
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_VSNPRINTF
size_t internal_memcpy(char *dest, const char *src, size_t destsize,
		       size_t srcsize) {
	size_t copied;

	copied = ((destsize) < (srcsize)) ? (destsize) : (srcsize);
	memcpy(dest, src, copied);

	return copied;
}

int tight_vsnprintf(char *str, size_t n, const char *format, va_list ap) {
	int written = 0;
	int tmpint, len;
	char buf[64]; /* Is it enough? */
	char *tmpstr;

	if (format == NULL || n < 1)
		return 0;

	while (*format != '\0' && written < n - 1) {
		if (*format != '%') {
			if (written < n) {
				str[written++] = *format++;
				continue;
			} else
				break;
		}

		format++;
		switch (*format) {
			case '\0':
				str[written++] = '%';
				continue;
			case 'd':
				tmpint = va_arg(ap, int);
				sprintf(buf, "%d", tmpint);
				len = strlen(buf);
				written += internal_memcpy (&str[written], buf,
							    len, n - written);
				break;
			case 's':
				tmpstr = va_arg(ap, char *);
				len = strlen(tmpstr);
				written += internal_memcpy (&str[written],
							    tmpstr, len,
							    n - written);
				break;
			/* Catch unimplemented stuff */
			default:
				fprintf(stderr, "Unimplemented format: %c\n",
					*format);
				abort();
		}
		format++;
	}

	str[written] = '\0';

	return written;
}
#endif /* HAVE_VSNPRINTF */

#ifndef HAVE_SNPRINTF
int tight_snprintf(char *str, size_t n, const char *format, ...) {
	va_list ap;
	int written;

	va_start(ap, format);
	written = vsnprintf(str, n, format, ap);
	va_end(ap);

	return written;
}
#endif /* HAVE_SNPRINTF */

