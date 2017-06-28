/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

// -=- Logger.cxx - support for the Logger and LogWriter classes

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <rfb/Logger.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>

using namespace rfb;

Logger* Logger::loggers = 0;

Logger::Logger(const char* name) : registered(false), m_name(name), m_next(0) {
}

Logger::~Logger() {
  // *** Should remove this logger here!
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"

void Logger::write(int level, const char *logname, const char* format,
                   va_list ap)
{
  // - Format the supplied data, and pass it to the
  //   actual log_message function
  //   The log level is included as a hint for loggers capable of representing
  //   different log levels in some way.
  char buf1[4096];
  vsnprintf(buf1, sizeof(buf1)-1, format, ap);
  buf1[sizeof(buf1)-1] = 0;
  write(level, logname, buf1);
}

void
Logger::registerLogger() {
  if (!registered) {
    registered = true;
    m_next = loggers;
    loggers=this;
  }
}
    
Logger*
Logger::getLogger(const char* name) {
  Logger* current = loggers;
  while (current) {
    if (strcasecmp(name, current->m_name) == 0) return current;
    current = current->m_next;
  }
  return 0;
}

void
Logger::listLoggers() {
  Logger* current = loggers;
  while (current) {
    printf("  %s\n", current->m_name);
    current = current->m_next;
  }
}


