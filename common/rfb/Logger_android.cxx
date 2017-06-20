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

#include <stdlib.h>
#include <string.h>

#include <utils/Log.h>

#include <rfb/util.h>
#include <rfb/Logger_android.h>
#include <rfb/LogWriter.h>

using namespace rfb;


Logger_Android::Logger_Android(const char* loggerName)
  : Logger(loggerName)
{
}

Logger_Android::~Logger_Android()
{
}

void Logger_Android::write(int level, const char *logname, const char *message)
{
  // Convert our priority level into syslog level
  if (level >= LogWriter::LEVEL_DEBUG) {
    ALOGD("%s: %s", logname, message);
  } else if (level >= LogWriter::LEVEL_INFO) {
    ALOGI("%s: %s", logname, message);
  } else if (level >= LogWriter::LEVEL_STATUS) {
    ALOGW("%s: %s", logname, message);
  } else {
    ALOGE("%s: %s", logname, message);
  }
}

static Logger_Android logger("android");

void rfb::initAndroidLogger() {
  logger.registerLogger();
}
