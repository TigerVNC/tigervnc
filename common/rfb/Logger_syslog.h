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

// -=- Logger_syslog - log to syslog

#ifndef __RFB_LOGGER_SYSLOG_H__
#define __RFB_LOGGER_SYSLOG_H__

#include <time.h>
#include <rfb/Logger.h>

namespace rfb {

  class Logger_Syslog : public Logger {
  public:
    Logger_Syslog(const char* loggerName);
    virtual ~Logger_Syslog();

    virtual void write(int level, const char *logname, const char *message);
  };

  void initSyslogLogger();
};

#endif
