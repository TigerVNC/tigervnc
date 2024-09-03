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

#include <stdexcept>
#include <string>

namespace rdr {

  struct PosixException : public std::runtime_error {
    int err;
    PosixException(const char* what_arg, int err_);
    PosixException(const std::string& what_arg, int err_);
  private:
    std::string strerror(int err_) const;
  };

#ifdef WIN32
  struct Win32Exception : public std::runtime_error {
    unsigned err;
    Win32Exception(const char* what_arg, unsigned err_);
    Win32Exception(const std::string& what_arg, unsigned err_);
  private:
    std::string strerror(unsigned err_) const;
  };
#endif

#ifdef WIN32
  struct SocketException : public Win32Exception {
    SocketException(const char* what_arg, unsigned err_) : Win32Exception(what_arg, err_) {}
    SocketException(const std::string& what_arg, unsigned err_) : Win32Exception(what_arg, err_) {}
  };
#else
  struct SocketException : public PosixException {
    SocketException(const char* what_arg, unsigned err_) : PosixException(what_arg, err_) {}
    SocketException(const std::string& what_arg, unsigned err_) : PosixException(what_arg, err_) {}
  };
#endif

  struct GAIException : public std::runtime_error {
    int err;
    GAIException(const char* s, int err_);
    GAIException(const std::string& s, int err_);
  private:
    std::string strerror(int err_) const;
  };

  struct EndOfStream : public std::runtime_error {
    EndOfStream() : std::runtime_error("End of stream") {}
  };

}

#endif
