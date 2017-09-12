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

//
// util.h - miscellaneous useful bits
//

#ifndef __RFB_UTIL_H__
#define __RFB_UTIL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <string.h>

struct timeval;

#ifdef __GNUC__
#  define __printf_attr(a, b) __attribute__((__format__ (__printf__, a, b)))
#else
#  define __printf_attr(a, b)
#endif // __GNUC__

#ifndef __unused
#  define __unused __attribute((__unused__))
#endif

namespace rfb {

  // -=- Class to handle cleanup of arrays of characters
  class CharArray {
  public:
    CharArray() : buf(0) {}
    CharArray(char* str) : buf(str) {} // note: assumes ownership
    CharArray(int len) {
      buf = new char[len];
    }
    ~CharArray() {
      delete [] buf;
    }
    void format(const char *fmt, ...) __printf_attr(2, 3);
    // Get the buffer pointer & clear it (i.e. caller takes ownership)
    char* takeBuf() {char* tmp = buf; buf = 0; return tmp;}
    void replaceBuf(char* b) {delete [] buf; buf = b;}
    char* buf;
  private:
    CharArray(const CharArray&);
    CharArray& operator=(const CharArray&);
  };

  char* strDup(const char* s);
  void strFree(char* s);

  // Returns true if split successful.  Returns false otherwise.
  // ALWAYS *copies* first part of string to out1 buffer.
  // If limiter not found, leaves out2 alone (null) and just copies to out1.
  // If out1 or out2 non-zero, calls strFree and zeroes them.
  // If fromEnd is true, splits at end of string rather than beginning.
  // Either out1 or out2 may be null, in which case the split will not return
  // that part of the string.  Obviously, setting both to 0 is not useful...
  bool strSplit(const char* src, const char limiter, char** out1, char** out2, bool fromEnd=false);

  // Returns true if src contains c
  bool strContains(const char* src, char c);

  // Copies src to dest, up to specified length-1, and guarantees termination
  void strCopy(char* dest, const char* src, int destlen);


  // HELPER functions for timeout handling

  // soonestTimeout() is a function to help work out the soonest of several
  //   timeouts.
  inline void soonestTimeout(int* timeout, int newTimeout) {
    if (newTimeout && (!*timeout || newTimeout < *timeout))
      *timeout = newTimeout;
  }

  // secsToMillis() turns seconds into milliseconds, capping the value so it
  //   can't wrap round and become -ve
  inline int secsToMillis(int secs) {
    return (secs < 0 || secs > (INT_MAX/1000) ? INT_MAX : secs * 1000);
  }

  // Returns time elapsed since given moment in milliseconds.
  unsigned msSince(const struct timeval *then);

  size_t siPrefix(long long value, const char *unit,
                  char *buffer, size_t maxlen, int precision=6);
  size_t iecPrefix(long long value, const char *unit,
                   char *buffer, size_t maxlen, int precision=6);
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
