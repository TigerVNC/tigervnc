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
#include <string.h>

#include <core/util.h>

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
}

void vncLogError(const char *name, const char *format, ...)
{
  LogWriter *vlog;
  va_list ap;
  vlog = LogWriter::getLogWriter(name);
  if (vlog == nullptr)
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
  if (vlog == nullptr)
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
  if (vlog == nullptr)
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
  if (vlog == nullptr)
    return;
  va_start(ap, format);
  vlog->vdebug(format, ap);
  va_end(ap);
}

int vncSetParam(const char *name, const char *value)
{
  if (value != nullptr)
    return rfb::Configuration::setParam(name, value);
  else {
    VoidParameter *param;
    param = rfb::Configuration::getParam(name);
    if (param == nullptr)
      return false;
    return param->setParam();
  }
}

char* vncGetParam(const char *name)
{
  VoidParameter *param;

  // Hack to avoid exposing password!
  if (strcasecmp(name, "Password") == 0)
    return nullptr;

  param = rfb::Configuration::getParam(name);
  if (param == nullptr)
    return nullptr;

  return strdup(param->getValueStr().c_str());
}

const char* vncGetParamDesc(const char *name)
{
  rfb::VoidParameter *param;

  param = rfb::Configuration::getParam(name);
  if (param == nullptr)
    return nullptr;

  return param->getDescription();
}

int vncIsParamBool(const char *name)
{
  VoidParameter *param;
  BoolParameter *bparam;

  param = rfb::Configuration::getParam(name);
  if (param == nullptr)
    return false;

  bparam = dynamic_cast<BoolParameter*>(param);
  if (bparam == nullptr)
    return false;

  return true;
}

int vncGetParamCount(void)
{
  int count;

  count = 0;
  for (VoidParameter *param: *rfb::Configuration::global())
    count++;

  return count;
}

char *vncGetParamList(void)
{
  int len;
  char *data, *ptr;

  len = 0;

  for (VoidParameter *param: *rfb::Configuration::global()) {
    int l = strlen(param->getName());
    if (l <= 255)
      len += l + 1;
  }

  data = (char*)malloc(len + 1);
  if (data == nullptr)
    return nullptr;

  ptr = data;
  for (VoidParameter *param: *rfb::Configuration::global()) {
    int l = strlen(param->getName());
    if (l <= 255) {
      *ptr++ = l;
      memcpy(ptr, param->getName(), l);
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

int vncHandleParamArg(int argc, char* argv[], int index)
{
  return rfb::Configuration::handleParamArg(argc, argv, index);
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
  } catch (std::exception& e) {
    return 1;
  }
  return 0;
}

char* vncConvertLF(const char* src, size_t bytes)
{
  try {
    return strdup(core::convertLF(src, bytes).c_str());
  } catch (...) {
    return nullptr;
  }
}

char* vncLatin1ToUTF8(const char* src, size_t bytes)
{
  try {
    return strdup(core::latin1ToUTF8(src, bytes).c_str());
  } catch (...) {
    return nullptr;
  }
}

char* vncUTF8ToLatin1(const char* src, size_t bytes)
{
  try {
    return strdup(core::utf8ToLatin1(src, bytes).c_str());
  } catch (...) {
    return nullptr;
  }
}

int vncIsValidUTF8(const char* str, size_t bytes)
{
  try {
    return core::isValidUTF8(str, bytes);
  } catch (...) {
    return 0;
  }
}
