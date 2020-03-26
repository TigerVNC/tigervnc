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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>

#include <rdr/Exception.h>
#include <rdr/TLSException.h>
#ifdef _WIN32
#include <tchar.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include <string.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

using namespace rdr;

Exception::Exception(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	(void) vsnprintf(str_, len, format, ap);
	va_end(ap);
}

GAIException::GAIException(const char* s, int err)
  : Exception("%s", s)
{
  strncat(str_, ": ", len-1-strlen(str_));
#ifdef _WIN32
  wchar_t *currStr = new wchar_t[len-strlen(str_)];
  wcsncpy(currStr, gai_strerrorW(err), len-1-strlen(str_));
  WideCharToMultiByte(CP_UTF8, 0, currStr, -1, str_+strlen(str_),
                      len-1-strlen(str_), 0, 0);
  delete [] currStr;
#else
  strncat(str_, gai_strerror(err), len-1-strlen(str_));
#endif
  strncat(str_, " (", len-1-strlen(str_));
  char buf[20];
#ifdef WIN32
  if (err < 0)
    sprintf(buf, "%x", err);
  else
#endif
    sprintf(buf,"%d",err);
  strncat(str_, buf, len-1-strlen(str_));
  strncat(str_, ")", len-1-strlen(str_));
}

SystemException::SystemException(const char* s, int err_)
  : Exception("%s", s), err(err_)
{
  strncat(str_, ": ", len-1-strlen(str_));
#ifdef _WIN32
  wchar_t *currStr = new wchar_t[len-strlen(str_)];
  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 0, err, 0, currStr, len-1-strlen(str_), 0);
  WideCharToMultiByte(CP_UTF8, 0, currStr, -1, str_+strlen(str_),
                      len-1-strlen(str_), 0, 0);
  delete [] currStr;

  int l = strlen(str_);
  if ((l >= 2) && (str_[l-2] == '\r') && (str_[l-1] == '\n'))
      str_[l-2] = 0;

#else
  strncat(str_, strerror(err), len-1-strlen(str_));
#endif
  strncat(str_, " (", len-1-strlen(str_));
  char buf[20];
#ifdef WIN32
  if (err < 0)
    sprintf(buf, "%x", err);
  else
#endif
    sprintf(buf,"%d",err);
  strncat(str_, buf, len-1-strlen(str_));
  strncat(str_, ")", len-1-strlen(str_));
}

