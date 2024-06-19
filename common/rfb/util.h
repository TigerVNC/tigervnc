/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2023 Pierre Ossman for Cendio AB
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

//
// util.h - miscellaneous useful bits
//

#ifndef __RFB_UTIL_H__
#define __RFB_UTIL_H__

#include <limits.h>
#include <stdint.h>

#include <string>
#include <vector>

struct timeval;

namespace rfb {

  // Formats according to printf(), with a dynamic allocation
  std::string format(const char *fmt, ...)
      __attribute__((__format__ (__printf__, 1, 2)));

  // Splits a string with the specified delimiter
  std::vector<std::string> split(const char* src,
                                 const char delimiter);

  // Conversion to and from a hex string

  void binToHex(const uint8_t* in, size_t inlen, char* out, size_t outlen);
  std::string binToHex(const uint8_t* in, size_t inlen);
  bool hexToBin(const char* in, size_t inlen, uint8_t* out, size_t outlen);
  std::vector<uint8_t> hexToBin(const char* in, size_t inlen);

  // Makes sure line endings are in a certain format

  std::string convertLF(const char* src, size_t bytes = (size_t)-1);
  std::string convertCRLF(const char* src, size_t bytes = (size_t)-1);

  // Convertions between various Unicode formats

  size_t ucs4ToUTF8(unsigned src, char dst[5]);
  size_t utf8ToUCS4(const char* src, size_t max, unsigned* dst);

  size_t ucs4ToUTF16(unsigned src, wchar_t dst[3]);
  size_t utf16ToUCS4(const wchar_t* src, size_t max, unsigned* dst);

  std::string latin1ToUTF8(const char* src, size_t bytes = (size_t)-1);
  std::string utf8ToLatin1(const char* src, size_t bytes = (size_t)-1);

  std::string utf16ToUTF8(const wchar_t* src, size_t units = (size_t)-1);
  std::wstring utf8ToUTF16(const char* src, size_t bytes = (size_t)-1);

  bool isValidUTF8(const char* str, size_t bytes = (size_t)-1);
  bool isValidUTF16(const wchar_t* wstr, size_t units = (size_t)-1);

  // HELPER functions for timeout handling

  // secsToMillis() turns seconds into milliseconds, capping the value so it
  //   can't wrap round and become -ve
  inline int secsToMillis(int secs) {
    return (secs < 0 || secs > (INT_MAX/1000) ? INT_MAX : secs * 1000);
  }

  // Returns time elapsed between two moments in milliseconds.
  unsigned msBetween(const struct timeval *first,
                     const struct timeval *second);

  // Returns time elapsed since given moment in milliseconds.
  unsigned msSince(const struct timeval *then);

  // Returns true if first happened before seconds
  bool isBefore(const struct timeval *first,
                const struct timeval *second);

  std::string siPrefix(long long value, const char *unit,
                       int precision=6);
  std::string iecPrefix(long long value, const char *unit,
                        int precision=6);
}

// Some platforms (e.g. Windows) include max() and min() macros in their
// standard headers, but they are also standard C++ template functions, so some
// C++ headers will undefine them.  So we steer clear of the names min and max
// and define __rfbmin and __rfbmax instead.

#ifndef __rfbmax
#define __rfbmax(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __rfbmin
#define __rfbmin(a,b) (((a) < (b)) ? (a) : (b))
#endif

#endif
