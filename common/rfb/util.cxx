/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman for Cendio AB
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

  char* convertLF(const char* src, size_t bytes)
  {
    char* buffer;
    size_t sz;

    char* out;
    const char* in;
    size_t in_len;

    // Always include space for a NULL
    sz = 1;

    // Compute output size
    in = src;
    in_len = bytes;
    while ((*in != '\0') && (in_len > 0)) {
      if (*in != '\r') {
        sz++;
        in++;
        in_len--;
        continue;
      }

      if ((in_len == 0) || (*(in+1) != '\n'))
        sz++;

      in++;
      in_len--;
    }

    // Alloc
    buffer = new char[sz];
    memset(buffer, 0, sz);

    // And convert
    out = buffer;
    in = src;
    in_len = bytes;
    while ((*in != '\0') && (in_len > 0)) {
      if (*in != '\r') {
        *out++ = *in++;
        in_len--;
        continue;
      }

      if ((in_len == 0) || (*(in+1) != '\n'))
        *out++ = '\n';

      in++;
      in_len--;
    }

    return buffer;
  }

  size_t ucs4ToUTF8(unsigned src, char* dst) {
    if (src < 0x80) {
      *dst++ = src;
      *dst++ = '\0';
      return 1;
    } else if (src < 0x800) {
      *dst++ = 0xc0 | (src >> 6);
      *dst++ = 0x80 | (src & 0x3f);
      *dst++ = '\0';
      return 2;
    } else if (src < 0x10000) {
      *dst++ = 0xe0 | (src >> 12);
      *dst++ = 0x80 | ((src >> 6) & 0x3f);
      *dst++ = 0x80 | (src & 0x3f);
      *dst++ = '\0';
      return 3;
    } else if (src < 0x110000) {
      *dst++ = 0xf0 | (src >> 18);
      *dst++ = 0x80 | ((src >> 12) & 0x3f);
      *dst++ = 0x80 | ((src >> 6) & 0x3f);
      *dst++ = 0x80 | (src & 0x3f);
      *dst++ = '\0';
      return 4;
    } else {
      return ucs4ToUTF8(0xfffd, dst);
    }
  }

  size_t utf8ToUCS4(const char* src, size_t max, unsigned* dst) {
    size_t count, consumed;

    *dst = 0xfffd;

    if (max == 0)
      return 0;

    consumed = 1;

    if ((*src & 0x80) == 0) {
      *dst = *src;
      count = 0;
    } else if ((*src & 0xe0) == 0xc0) {
      *dst = *src & 0x1f;
      count = 1;
    } else if ((*src & 0xf0) == 0xe0) {
      *dst = *src & 0x0f;
      count = 2;
    } else if ((*src & 0xf8) == 0xf0) {
      *dst = *src & 0x07;
      count = 3;
    } else {
      // Invalid sequence, consume all continuation characters
      src++;
      max--;
      while ((max-- > 0) && ((*src++ & 0xc0) == 0x80))
        consumed++;
      return consumed;
    }

    src++;
    max--;

    while (count--) {
      // Invalid or truncated sequence?
      if ((max == 0) || ((*src & 0xc0) != 0x80)) {
        *dst = 0xfffd;
        return consumed;
      }

      *dst <<= 6;
      *dst |= *src & 0x3f;

      src++;
      max--;
    }

    return consumed;
  }

  char* latin1ToUTF8(const char* src, size_t bytes) {
    char* buffer;
    size_t sz;

    char* out;
    const char* in;
    size_t in_len;

    // Always include space for a NULL
    sz = 1;

    // Compute output size
    in = src;
    in_len = bytes;
    while ((*in != '\0') && (in_len > 0)) {
      char buf[5];
      sz += ucs4ToUTF8(*in, buf);
      in++;
      in_len--;
    }

    // Alloc
    buffer = new char[sz];
    memset(buffer, 0, sz);

    // And convert
    out = buffer;
    in = src;
    in_len = bytes;
    while ((*in != '\0') && (in_len > 0)) {
      out += ucs4ToUTF8(*in, out);
      in++;
      in_len--;
    }

    return buffer;
  }

  char* utf8ToLatin1(const char* src, size_t bytes) {
    char* buffer;
    size_t sz;

    char* out;
    const char* in;
    size_t in_len;

    // Always include space for a NULL
    sz = 1;

    // Compute output size
    in = src;
    in_len = bytes;
    while ((*in != '\0') && (in_len > 0)) {
      size_t len;
      unsigned ucs;

      len = utf8ToUCS4(in, in_len, &ucs);
      in += len;
      in_len -= len;
      sz++;
    }

    // Alloc
    buffer = new char[sz];
    memset(buffer, 0, sz);

    // And convert
    out = buffer;
    in = src;
    in_len = bytes;
    while ((*in != '\0') && (in_len > 0)) {
      size_t len;
      unsigned ucs;

      len = utf8ToUCS4(in, in_len, &ucs);
      in += len;
      in_len -= len;

      if (ucs > 0xff)
        *out++ = '?';
      else
        *out++ = (unsigned char)ucs;
    }

    return buffer;
  }

  unsigned msBetween(const struct timeval *first,
                     const struct timeval *second)
  {
    unsigned diff;

    diff = (second->tv_sec - first->tv_sec) * 1000;

    diff += second->tv_usec / 1000;
    diff -= first->tv_usec / 1000;

    return diff;
  }

  unsigned msSince(const struct timeval *then)
  {
    struct timeval now;

    gettimeofday(&now, NULL);

    return msBetween(then, &now);
  }

  bool isBefore(const struct timeval *first,
                const struct timeval *second)
  {
    if (first->tv_sec < second->tv_sec)
      return true;
    if (first->tv_sec > second->tv_sec)
      return false;
    if (first->tv_usec < second->tv_usec)
      return true;
    return false;
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
