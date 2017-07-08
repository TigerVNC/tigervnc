/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

/*
 * The following applies to stcasecmp and strncasecmp implementations:
 *
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

#include <rfb/util.h>

namespace rfb {

  void CharArray::format(const char *fmt, ...) {
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    delete [] buf;

    if (len < 0) {
      buf = new char[1];
      buf[0] = '\0';
      return;
    }

    buf = new char[len+1];

    va_start(ap, fmt);
    vsnprintf(buf, len+1, fmt, ap);
    va_end(ap);
  }

  char* strDup(const char* s) {
    if (!s) return 0;
    int l = strlen(s);
    char* r = new char[l+1];
    memcpy(r, s, l+1);
    return r;
  };

  void strFree(char* s) {
    delete [] s;
  }


  bool strSplit(const char* src, const char limiter, char** out1, char** out2, bool fromEnd) {
    CharArray out1old, out2old;
    if (out1) out1old.buf = *out1;
    if (out2) out2old.buf = *out2;
    int len = strlen(src);
    int i=0, increment=1, limit=len;
    if (fromEnd) {
      i=len-1; increment = -1; limit = -1;
    }
    while (i!=limit) {
      if (src[i] == limiter) {
        if (out1) {
          *out1 = new char[i+1];
          if (i) memcpy(*out1, src, i);
          (*out1)[i] = 0;
        }
        if (out2) {
          *out2 = new char[len-i];
          if (len-i-1) memcpy(*out2, &src[i+1], len-i-1);
          (*out2)[len-i-1] = 0;
        }
        return true;
      }
      i+=increment;
    }
    if (out1) *out1 = strDup(src);
    if (out2) *out2 = 0;
    return false;
  }

  bool strContains(const char* src, char c) {
    int l=strlen(src);
    for (int i=0; i<l; i++)
      if (src[i] == c) return true;
    return false;
  }

  void strCopy(char* dest, const char* src, int destlen) {
    if (src)
      strncpy(dest, src, destlen-1);
    dest[src ? destlen-1 : 0] = 0;
  }

  unsigned msSince(const struct timeval *then)
  {
    struct timeval now;
    unsigned diff;

    gettimeofday(&now, NULL);

    diff = (now.tv_sec - then->tv_sec) * 1000;

    diff += now.tv_usec / 1000;
    diff -= then->tv_usec / 1000;

    return diff;
  }

  static size_t doPrefix(long long value, const char *unit,
                         char *buffer, size_t maxlen,
                         unsigned divisor, const char **prefixes,
                         size_t prefixCount, int precision) {
    double newValue;
    size_t prefix, len;

    newValue = value;
    prefix = 0;
    while (newValue >= divisor) {
      if (prefix >= prefixCount)
        break;
      newValue /= divisor;
      prefix++;
    }

    len = snprintf(buffer, maxlen, "%.*g %s%s", precision, newValue,
                   (prefix == 0) ? "" : prefixes[prefix-1], unit);
    buffer[maxlen-1] = '\0';

    return len;
  }

  static const char *siPrefixes[] =
    { "k", "M", "G", "T", "P", "E", "Z", "Y" };
  static const char *iecPrefixes[] =
    { "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi" };

  size_t siPrefix(long long value, const char *unit,
                  char *buffer, size_t maxlen, int precision) {
    return doPrefix(value, unit, buffer, maxlen, 1000, siPrefixes,
                    sizeof(siPrefixes)/sizeof(*siPrefixes),
                    precision);
  }

  size_t iecPrefix(long long value, const char *unit,
                   char *buffer, size_t maxlen, int precision) {
    return doPrefix(value, unit, buffer, maxlen, 1024, iecPrefixes,
                    sizeof(iecPrefixes)/sizeof(*iecPrefixes),
                    precision);
  }
};
