/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2010 TigerVNC Team
 * Copyright 2015-2024 Pierre Ossman for Cendio AB
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

  class PosixException : public std::runtime_error {
  public:
    int err;
    PosixException(const char* what_arg, int err_);
    PosixException(const std::string& what_arg, int err_);
  private:
    std::string strerror(int err_) const;
  };

#ifdef WIN32
  class Win32Exception : public std::runtime_error {
  public:
    unsigned err;
    Win32Exception(const char* what_arg, unsigned err_);
    Win32Exception(const std::string& what_arg, unsigned err_);
  private:
    std::string strerror(unsigned err_) const;
  };
#endif

#ifdef WIN32
  class SocketException : public Win32Exception {
  public:
    SocketException(const char* what_arg, unsigned err_) : Win32Exception(what_arg, err_) {}
    SocketException(const std::string& what_arg, unsigned err_) : Win32Exception(what_arg, err_) {}
  };
#else
  class SocketException : public PosixException {
  public:
    SocketException(const char* what_arg, unsigned err_) : PosixException(what_arg, err_) {}
    SocketException(const std::string& what_arg, unsigned err_) : PosixException(what_arg, err_) {}
  };
#endif

  class GAIException : public std::runtime_error {
  public:
    int err;
    GAIException(const char* s, int err_);
    GAIException(const std::string& s, int err_);
  private:
    std::string strerror(int err_) const;
  };

  class EndOfStream : public std::runtime_error {
  public:
    EndOfStream() : std::runtime_error("End of stream") {}
  };

}

#endif
