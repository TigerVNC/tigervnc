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

#include <rdr/Exception.h>
#include <rdr/TLSException.h>
#include <rfb/util.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include <string.h>

using namespace rdr;


GAIException::GAIException(const char* s, int err_)
  : Exception(rfb::format("%s: %s (%d)", s, strerror(err_).c_str(), err_)),
    err(err_)
{
}

std::string GAIException::strerror(int err_) const
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

PosixException::PosixException(const char* s, int err_)
  : Exception(rfb::format("%s: %s (%d)", s, strerror(err_).c_str(), err_)),
    err(err_)
{
}

std::string PosixException::strerror(int err_) const
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
Win32Exception::Win32Exception(const char* s, unsigned err_)
  : Exception(rfb::format("%s: %s (%d)", s, strerror(err_).c_str(), err_)),
    err(err_)
{
}

std::string Win32Exception::strerror(unsigned err_) const
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
