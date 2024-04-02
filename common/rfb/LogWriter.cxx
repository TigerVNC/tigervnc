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

// -=- LogWriter.cxx - client-side logging interface

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>
#include <rfb/util.h>
#include <stdlib.h>

rfb::LogParameter rfb::logParams;

using namespace rfb;


LogWriter::LogWriter(const char* name)
  : m_name(name), m_level(0), m_log(nullptr), m_next(log_writers) {
  log_writers = this;
}

LogWriter::~LogWriter() {
  // *** Should remove this logger here!
}

void LogWriter::setLog(Logger *logger) {
  m_log = logger;
}

void LogWriter::setLevel(int level) {
  m_level = level;
}

void
LogWriter::listLogWriters(int /*width*/) {
  // *** make this respect width...
  LogWriter* current = log_writers;
  fprintf(stderr, "  ");
  while (current) {
    fprintf(stderr, "%s", current->m_name);
    current = current->m_next;
    if (current) fprintf(stderr, ", ");
  }
  fprintf(stderr, "\n");
}

LogWriter* LogWriter::log_writers;

LogWriter*
LogWriter::getLogWriter(const char* name) {
  LogWriter* current = log_writers;
  while (current) {
    if (strcasecmp(name, current->m_name) == 0) return current;
      current = current->m_next;
    }
  return nullptr;
}

bool LogWriter::setLogParams(const char* params) {
  std::vector<std::string> parts;
  parts = split(params, ':');
  if (parts.size() != 3) {
    fprintf(stderr,"failed to parse log params:%s\n",params);
    return false;
  }
  int level = atoi(parts[2].c_str());
  Logger* logger = nullptr;
  if (!parts[1].empty()) {
    logger = Logger::getLogger(parts[1].c_str());
    if (!logger)
      fprintf(stderr, "no logger found! %s\n", parts[1].c_str());
  }
  if (parts[0] == "*") {
    LogWriter* current = log_writers;
    while (current) {
      current->setLog(logger);
      current->setLevel(level);
      current = current->m_next;
    }
    return true;
  } else {
    LogWriter* logwriter = getLogWriter(parts[0].c_str());
    if (!logwriter) {
      fprintf(stderr, "no logwriter found! %s\n", parts[0].c_str());
    } else {
      logwriter->setLog(logger);
      logwriter->setLevel(level);
      return true;
    }
  }
  return false;
}


LogParameter::LogParameter()
  : StringParameter("Log",
    "Specifies which log output should be directed to "
    "which target logger, and the level of output to log. "
    "Format is <log>:<target>:<level>[, ...].",
    "") {
}

bool LogParameter::setParam(const char* v) {
  if (immutable) return true;
  LogWriter::setLogParams("*::0");
  StringParameter::setParam(v);
  std::vector<std::string> parts;
  parts = split(v, ',');
  for (size_t i = 0; i < parts.size(); i++) {
    if (parts[i].empty())
        continue;
    if (!LogWriter::setLogParams(parts[i].c_str()))
      return false;
  }
  return true;
}
