/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2010 TigerVNC Team
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

#ifndef __RDR_EXCEPTION_H__
#define __RDR_EXCEPTION_H__

#ifdef __GNUC__
#  define __printf_attr(a, b) __attribute__((__format__ (__printf__, a, b)))
#else
#  define __printf_attr(a, b)
#endif // __GNUC__

namespace rdr {

  struct Exception {
    enum { len = 256 };
    char str_[len];
    Exception(const char *format = 0, ...) __printf_attr(2, 3);
    virtual ~Exception() {}
    virtual const char* str() const { return str_; }
  };

  struct SystemException : public Exception {
    int err;
    SystemException(const char* s, int err_);
  };

  struct GAIException : public Exception {
    int err;
    GAIException(const char* s, int err_);
  };

  struct EndOfStream : public Exception {
    EndOfStream() : Exception("End of stream") {}
  };

}

#endif
