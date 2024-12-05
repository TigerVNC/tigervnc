/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2010 TigerVNC Team
 * Copyright 2014-2024 Pierre Ossman for Cendio AB
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

#include <stdio.h>
#include <stdarg.h>

#include <core/Exception.h>
#include <core/string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include <string.h>

using namespace core;


getaddrinfo_error::getaddrinfo_error(const char* s, int err_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", s,
                                    strerror(err_).c_str(), err_)),
    err(err_)
{
}

getaddrinfo_error::getaddrinfo_error(const std::string& s,
                                     int err_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", s.c_str(),
                                    strerror(err_).c_str(), err_)),
    err(err_)
{
}

std::string getaddrinfo_error::strerror(int err_) const noexcept
{
#ifdef _WIN32
  char str[256];

  WideCharToMultiByte(CP_UTF8, 0, gai_strerrorW(err_), -1, str,
                      sizeof(str), nullptr, nullptr);

  return str;
#else
  return gai_strerror(err_);
#endif
}

posix_error::posix_error(const char* what_arg, int err_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", what_arg,
                                    strerror(err_).c_str(), err_)),
    err(err_)
{
}

posix_error::posix_error(const std::string& what_arg, int err_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", what_arg.c_str(),
                                    strerror(err_).c_str(), err_)),
    err(err_)
{
}

std::string posix_error::strerror(int err_) const noexcept
{
#ifdef _WIN32
  char str[256];

  WideCharToMultiByte(CP_UTF8, 0, _wcserror(err_), -1, str,
                      sizeof(str), nullptr, nullptr);

  return str;
#else
  return ::strerror(err_);
#endif
}

#ifdef WIN32
win32_error::win32_error(const char* what_arg, unsigned err_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", what_arg,
                                    strerror(err_).c_str(), err_)),
    err(err_)
{
}

win32_error::win32_error(const std::string& what_arg,
                         unsigned err_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", what_arg.c_str(),
                                    strerror(err_).c_str(), err_)),
    err(err_)
{
}

std::string win32_error::strerror(unsigned err_) const noexcept
{
  wchar_t wstr[256];
  char str[256];

  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, err_, 0, wstr, sizeof(wstr), nullptr);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str,
                      sizeof(str), nullptr, nullptr);

  int l = strlen(str);
  if ((l >= 2) && (str[l-2] == '\r') && (str[l-1] == '\n'))
      str[l-2] = 0;

  return str;
}
#endif
