/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman for Cendio AB
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

#include <stdlib.h>

#include <network/TcpSocket.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/Logger_stdio.h>
#include <rfb/Logger_syslog.h>

#include "RFBGlue.h"

using namespace rfb;

// Loggers used by C code must be created here
static LogWriter inputLog("Input");
static LogWriter selectionLog("Selection");

void vncInitRFB(void)
{
  rfb::initStdIOLoggers();
  rfb::initSyslogLogger();
  rfb::LogWriter::setLogParams("*:stderr:30");
  rfb::Configuration::enableServerParams();
}

void vncLogError(const char *name, const char *format, ...)
{
  LogWriter *vlog;
  va_list ap;
  vlog = LogWriter::getLogWriter(name);
  if (vlog == NULL)
    return;
  va_start(ap, format);
  vlog->verror(format, ap);
  va_end(ap);
}

void vncLogStatus(const char *name, const char *format, ...)
{
  LogWriter *vlog;
  va_list ap;
  vlog = LogWriter::getLogWriter(name);
  if (vlog == NULL)
    return;
  va_start(ap, format);
  vlog->vstatus(format, ap);
  va_end(ap);
}

void vncLogInfo(const char *name, const char *format, ...)
{
  LogWriter *vlog;
  va_list ap;
  vlog = LogWriter::getLogWriter(name);
  if (vlog == NULL)
    return;
  va_start(ap, format);
  vlog->vinfo(format, ap);
  va_end(ap);
}

void vncLogDebug(const char *name, const char *format, ...)
{
  LogWriter *vlog;
  va_list ap;
  vlog = LogWriter::getLogWriter(name);
  if (vlog == NULL)
    return;
  va_start(ap, format);
  vlog->vdebug(format, ap);
  va_end(ap);
}

int vncSetParam(const char *name, const char *value)
{
  if (value != NULL)
    return rfb::Configuration::setParam(name, value);
  else {
    VoidParameter *param;
    param = rfb::Configuration::getParam(name);
    if (param == NULL)
      return false;
    return param->setParam();
  }
}

int vncSetParamSimple(const char *nameAndValue)
{
  return rfb::Configuration::setParam(nameAndValue);
}

char* vncGetParam(const char *name)
{
  rfb::VoidParameter *param;
  char *value;
  char *ret;

  // Hack to avoid exposing password!
  if (strcasecmp(name, "Password") == 0)
    return NULL;

  param = rfb::Configuration::getParam(name);
  if (param == NULL)
    return NULL;

  value = param->getValueStr();
  if (value == NULL)
    return NULL;

  ret = strdup(value);

  delete [] value;

  return ret;
}

const char* vncGetParamDesc(const char *name)
{
  rfb::VoidParameter *param;

  param = rfb::Configuration::getParam(name);
  if (param == NULL)
    return NULL;

  return param->getDescription();
}

int vncIsParamBool(const char *name)
{
  VoidParameter *param;
  BoolParameter *bparam;

  param = rfb::Configuration::getParam(name);
  if (param == NULL)
    return false;

  bparam = dynamic_cast<BoolParameter*>(param);
  if (bparam == NULL)
    return false;

  return true;
}

int vncGetParamCount(void)
{
  int count;

  count = 0;
  for (ParameterIterator i; i.param; i.next())
    count++;

  return count;
}

char *vncGetParamList(void)
{
  int len;
  char *data, *ptr;

  len = 0;

  for (ParameterIterator i; i.param; i.next()) {
    int l = strlen(i.param->getName());
    if (l <= 255)
      len += l + 1;
  }

  data = (char*)malloc(len + 1);
  if (data == NULL)
    return NULL;

  ptr = data;
  for (ParameterIterator i; i.param; i.next()) {
    int l = strlen(i.param->getName());
    if (l <= 255) {
      *ptr++ = l;
      memcpy(ptr, i.param->getName(), l);
      ptr += l;
    }
  }
  *ptr = '\0';

  return data;
}

void vncListParams(int width, int nameWidth)
{
  rfb::Configuration::listParams(width, nameWidth);
}

int vncGetSocketPort(int fd)
{
  return network::getSockPort(fd);
}

int vncIsTCPPortUsed(int port)
{
  try {
    // Attempt to create TCPListeners on that port.
    std::list<network::SocketListener*> dummy;
    network::createTcpListeners (&dummy, 0, port);
    while (!dummy.empty()) {
      delete dummy.back();
      dummy.pop_back();
    }
  } catch (rdr::Exception& e) {
    return 1;
  }
  return 0;
}

char* vncConvertLF(const char* src, size_t bytes)
{
  try {
    return convertLF(src, bytes);
  } catch (...) {
    return NULL;
  }
}

char* vncLatin1ToUTF8(const char* src, size_t bytes)
{
  try {
    return latin1ToUTF8(src, bytes);
  } catch (...) {
    return NULL;
  }
}

char* vncUTF8ToLatin1(const char* src, size_t bytes)
{
  try {
    return utf8ToLatin1(src, bytes);
  } catch (...) {
    return NULL;
  }
}

void vncStrFree(char* str)
{
  strFree(str);
}
