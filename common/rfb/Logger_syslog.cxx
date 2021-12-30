/* Copyright (C) 2015 TigerVNC
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

// -=- Logger_syslog.cxx - Logger instance for a syslog

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <rfb/util.h>
#include <rfb/Logger_syslog.h>
#include <rfb/LogWriter.h>

using namespace rfb;


Logger_Syslog::Logger_Syslog(const char* loggerName)
  : Logger(loggerName)
{
  openlog(0, LOG_CONS | LOG_PID, LOG_USER);
}

Logger_Syslog::~Logger_Syslog()
{
  closelog();
}

void Logger_Syslog::write(int level, const char *logname, const char *message)
{
  // Convert our priority level into syslog level
  int priority;
  if (level >= LogWriter::LEVEL_DEBUG) {
    priority = LOG_DEBUG;
  } else if (level >= LogWriter::LEVEL_INFO) {
    priority = LOG_INFO;
  } else if (level >= LogWriter::LEVEL_STATUS) {
    priority = LOG_NOTICE;
  } else {
    priority = LOG_ERR;
  }

  syslog(priority, "%s: %s", logname, message);
}

static Logger_Syslog logger("syslog");

void rfb::initSyslogLogger() {
  logger.registerLogger();
}
