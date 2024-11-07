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

  class posix_error : public std::runtime_error {
  public:
    int err;
    posix_error(const char* what_arg, int err_);
    posix_error(const std::string& what_arg, int err_);
  private:
    std::string strerror(int err_) const;
  };

#ifdef WIN32
  class win32_error : public std::runtime_error {
  public:
    unsigned err;
    win32_error(const char* what_arg, unsigned err_);
    win32_error(const std::string& what_arg, unsigned err_);
  private:
    std::string strerror(unsigned err_) const;
  };
#endif

#ifdef WIN32
  class socket_error : public win32_error {
  public:
    socket_error(const char* what_arg, unsigned err_) : win32_error(what_arg, err_) {}
    socket_error(const std::string& what_arg, unsigned err_) : win32_error(what_arg, err_) {}
  };
#else
  class socket_error : public posix_error {
  public:
    socket_error(const char* what_arg, unsigned err_) : posix_error(what_arg, err_) {}
    socket_error(const std::string& what_arg, unsigned err_) : posix_error(what_arg, err_) {}
  };
#endif

  class getaddrinfo_error : public std::runtime_error {
  public:
    int err;
    getaddrinfo_error(const char* s, int err_);
    getaddrinfo_error(const std::string& s, int err_);
  private:
    std::string strerror(int err_) const;
  };

  class end_of_stream : public std::runtime_error {
  public:
    end_of_stream() : std::runtime_error("End of stream") {}
  };

}

#endif
