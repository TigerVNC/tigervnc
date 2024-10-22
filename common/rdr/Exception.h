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

  struct posix_error : public std::runtime_error {
    int err;
    posix_error(const char* what_arg, int err_);
    posix_error(const std::string& what_arg, int err_);
  private:
    std::string strerror(int err_) const;
  };

#ifdef WIN32
  struct win32_error : public std::runtime_error {
    unsigned err;
    win32_error(const char* what_arg, unsigned err_);
    win32_error(const std::string& what_arg, unsigned err_);
  private:
    std::string strerror(unsigned err_) const;
  };
#endif

#ifdef WIN32
  struct socket_error : public win32_error {
    socket_error(const char* what_arg, unsigned err_) : win32_error(what_arg, err_) {}
    socket_error(const std::string& what_arg, unsigned err_) : win32_error(what_arg, err_) {}
  };
#else
  struct socket_error : public posix_error {
    socket_error(const char* what_arg, unsigned err_) : posix_error(what_arg, err_) {}
    socket_error(const std::string& what_arg, unsigned err_) : posix_error(what_arg, err_) {}
  };
#endif

  struct getaddrinfo_error : public std::runtime_error {
    int err;
    getaddrinfo_error(const char* s, int err_);
    getaddrinfo_error(const std::string& s, int err_);
  private:
    std::string strerror(int err_) const;
  };

  struct EndOfStream : public std::runtime_error {
    EndOfStream() : std::runtime_error("End of stream") {}
  };

}

#endif
