/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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

#include <stdio.h>
#include <string.h>

namespace rdr {

  struct Exception {
    enum { len = 256 };
    char str_[len];
    char type_[len];
    Exception(const char* s=0, const char* e="rdr::Exception") {
      str_[0] = 0;
      if (s)
        strncat(str_, s, len-1);
      else
        strcat(str_, "Exception");
      type_[0] = 0;
      strncat(type_, e, len-1);
    }
    virtual const char* str() const { return str_; }
    virtual const char* type() const { return type_; }
  };

  struct SystemException : public Exception {
    int err;
    SystemException(const char* s, int err_);
  }; 

  struct TimedOut : public Exception {
    TimedOut(const char* s="Timed out") : Exception(s,"rdr::TimedOut") {}
  };
 
  struct EndOfStream : public Exception {
    EndOfStream(const char* s="End of stream")
      : Exception(s,"rdr::EndOfStream") {}
  };
}

#endif
