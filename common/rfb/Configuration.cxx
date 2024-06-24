/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
 * Copyright 2011-2022 Pierre Ossman for Cendio AB
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

// -=- Configuration.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <os/Mutex.h>

#include <rfb/util.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#define LOCK_CONFIG os::AutoMutex a(mutex)

#include <rdr/HexOutStream.h>
#include <rdr/HexInStream.h>

using namespace rfb;

static LogWriter vlog("Config");


// -=- The Global/server/viewer Configuration objects
Configuration* Configuration::global_ = nullptr;
Configuration* Configuration::server_ = nullptr;
Configuration* Configuration::viewer_ = nullptr;

Configuration* Configuration::global() {
  if (!global_)
    global_ = new Configuration("Global");
  return global_;
}

Configuration* Configuration::server() {
  if (!server_)
    server_ = new Configuration("Server");
  return server_;
}

Configuration* Configuration::viewer() {
  if (!viewer_)
    viewer_ = new Configuration("Viewer");
  return viewer_;
}

// -=- Configuration implementation

bool Configuration::set(const char* n, const char* v, bool immutable) {
  return set(n, strlen(n), v, immutable);
}

bool Configuration::set(const char* paramName, int len,
                             const char* val, bool immutable)
{
  VoidParameter* current = head;
  while (current) {
    if ((int)strlen(current->getName()) == len &&
        strncasecmp(current->getName(), paramName, len) == 0)
    {
      bool b = current->setParam(val);
      if (b && immutable) 
	current->setImmutable();
      return b;
    }
    current = current->_next;
  }
  return _next ? _next->set(paramName, len, val, immutable) : false;
}

bool Configuration::set(const char* config, bool immutable) {
  bool hyphen = false;
  if (config[0] == '-') {
    hyphen = true;
    config++;
    if (config[0] == '-') config++; // allow gnu-style --<option>
  }
  const char* equal = strchr(config, '=');
  if (equal) {
    return set(config, equal-config, equal+1, immutable);
  } else if (hyphen) {
    VoidParameter* current = head;
    while (current) {
      if (strcasecmp(current->getName(), config) == 0) {
        bool b = current->setParam();
        if (b && immutable) 
	  current->setImmutable();
        return b;
      }
      current = current->_next;
    }
  }    
  return _next ? _next->set(config, immutable) : false;
}

VoidParameter* Configuration::get(const char* param)
{
  VoidParameter* current = head;
  while (current) {
    if (strcasecmp(current->getName(), param) == 0)
      return current;
    current = current->_next;
  }
  return _next ? _next->get(param) : nullptr;
}

void Configuration::list(int width, int nameWidth) {
  VoidParameter* current = head;

  fprintf(stderr, "%s Parameters:\n", name.c_str());
  while (current) {
    std::string def_str = current->getDefaultStr();
    const char* desc = current->getDescription();
    fprintf(stderr,"  %-*s -", nameWidth, current->getName());
    int column = strlen(current->getName());
    if (column < nameWidth) column = nameWidth;
    column += 4;
    while (true) {
      const char* s = strchr(desc, ' ');
      int wordLen;
      if (s) wordLen = s-desc;
      else wordLen = strlen(desc);

      if (column + wordLen + 1 > width) {
        fprintf(stderr,"\n%*s",nameWidth+4,"");
        column = nameWidth+4;
      }
      fprintf(stderr," %.*s",wordLen,desc);
      column += wordLen + 1;
      desc += wordLen + 1;
      if (!s) break;
    }

    if (!def_str.empty()) {
      if (column + (int)def_str.size() + 11 > width)
        fprintf(stderr,"\n%*s",nameWidth+4,"");
      fprintf(stderr," (default=%s)\n",def_str.c_str());
    } else {
      fprintf(stderr,"\n");
    }
    current = current->_next;
  }

  if (_next)
    _next->list(width, nameWidth);
}


bool Configuration::remove(const char* param) {
  VoidParameter *current = head;
  VoidParameter **prevnext = &head;

  while (current) {
    if (strcasecmp(current->getName(), param) == 0) {
      *prevnext = current->_next;
      return true;
    }
    prevnext = &current->_next;
    current = current->_next;
  }

  return false;
}


// -=- VoidParameter

VoidParameter::VoidParameter(const char* name_, const char* desc_,
			     ConfigurationObject co)
  : immutable(false), name(name_), description(desc_)
{
  Configuration *conf = nullptr;

  switch (co) {
  case ConfGlobal: conf = Configuration::global();
    break;
  case ConfServer: conf = Configuration::server();
    break;
  case ConfViewer: conf = Configuration::viewer();
    break;
  }

  _next = conf->head;
  conf->head = this;

  mutex = new os::Mutex();
}

VoidParameter::~VoidParameter() {
  delete mutex;
}

const char*
VoidParameter::getName() const {
  return name;
}

const char*
VoidParameter::getDescription() const {
  return description;
}

bool VoidParameter::setParam() {
  return false;
}

bool VoidParameter::isBool() const {
  return false;
}

void
VoidParameter::setImmutable() {
  vlog.debug("set immutable %s", getName());
  immutable = true;
}

// -=- AliasParameter

AliasParameter::AliasParameter(const char* name_, const char* desc_,
                               VoidParameter* param_, ConfigurationObject co)
  : VoidParameter(name_, desc_, co), param(param_) {
}

bool
AliasParameter::setParam(const char* v) {
  return param->setParam(v);
}

bool AliasParameter::setParam() {
  return param->setParam();
}

std::string AliasParameter::getDefaultStr() const {
  return "";
}

std::string AliasParameter::getValueStr() const {
  return param->getValueStr();
}

bool AliasParameter::isBool() const {
  return param->isBool();
}

void
AliasParameter::setImmutable() {
  vlog.debug("set immutable %s (Alias)", getName());
  param->setImmutable();
}


// -=- BoolParameter

BoolParameter::BoolParameter(const char* name_, const char* desc_, bool v,
			     ConfigurationObject co)
: VoidParameter(name_, desc_, co), value(v), def_value(v) {
}

bool
BoolParameter::setParam(const char* v) {
  if (immutable) return true;

  if (*v == 0 || strcasecmp(v, "1") == 0 || strcasecmp(v, "on") == 0
      || strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0)
    setParam(true);
  else if (strcasecmp(v, "0") == 0 || strcasecmp(v, "off") == 0
           || strcasecmp(v, "false") == 0 || strcasecmp(v, "no") == 0)
    setParam(false);
  else {
    vlog.error("Bool parameter %s: invalid value '%s'", getName(), v);
    return false;
  }

  return true;
}

bool BoolParameter::setParam() {
  setParam(true);
  return true;
}

void BoolParameter::setParam(bool b) {
  if (immutable) return;
  value = b;
  vlog.debug("set %s(Bool) to %d", getName(), value);
}

std::string BoolParameter::getDefaultStr() const {
  return def_value ? "1" : "0";
}

std::string BoolParameter::getValueStr() const {
  return value ? "1" : "0";
}

bool BoolParameter::isBool() const {
  return true;
}

BoolParameter::operator bool() const {
  return value;
}

// -=- IntParameter

IntParameter::IntParameter(const char* name_, const char* desc_, int v,
                           int minValue_, int maxValue_, ConfigurationObject co)
  : VoidParameter(name_, desc_, co), value(v), def_value(v),
    minValue(minValue_), maxValue(maxValue_)
{
}

bool
IntParameter::setParam(const char* v) {
  if (immutable) return true;
  return setParam(strtol(v, nullptr, 0));
}

bool
IntParameter::setParam(int v) {
  if (immutable) return true;
  vlog.debug("set %s(Int) to %d", getName(), v);
  if (v < minValue || v > maxValue)
    return false;
  value = v;
  return true;
}

std::string IntParameter::getDefaultStr() const {
  char result[16];
  sprintf(result, "%d", def_value);
  return result;
}

std::string IntParameter::getValueStr() const {
  char result[16];
  sprintf(result, "%d", value);
  return result;
}

IntParameter::operator int() const {
  return value;
}

// -=- StringParameter

StringParameter::StringParameter(const char* name_, const char* desc_,
                                 const char* v, ConfigurationObject co)
  : VoidParameter(name_, desc_, co), value(v), def_value(v)
{
  if (!v) {
    vlog.error("Default value <null> for %s not allowed",name_);
    throw rfb::Exception("Default value <null> not allowed");
  }
}

StringParameter::~StringParameter() {
}

bool StringParameter::setParam(const char* v) {
  LOCK_CONFIG;
  if (immutable) return true;
  if (!v)
    throw rfb::Exception("setParam(<null>) not allowed");
  vlog.debug("set %s(String) to %s", getName(), v);
  value = v;
  return true;
}

std::string StringParameter::getDefaultStr() const {
  return def_value;
}

std::string StringParameter::getValueStr() const {
  LOCK_CONFIG;
  return value;
}

StringParameter::operator const char *() const {
  return value.c_str();
}

// -=- BinaryParameter

BinaryParameter::BinaryParameter(const char* name_, const char* desc_,
				 const uint8_t* v, size_t l, ConfigurationObject co)
: VoidParameter(name_, desc_, co),
  value(nullptr), length(0), def_value(nullptr), def_length(0) {
  if (l) {
    assert(v);
    value = new uint8_t[l];
    length = l;
    memcpy(value, v, l);
    def_value = new uint8_t[l];
    def_length = l;
    memcpy(def_value, v, l);
  }
}
BinaryParameter::~BinaryParameter() {
  delete [] value;
  delete [] def_value;
}

bool BinaryParameter::setParam(const char* v) {
  if (immutable) return true;
  std::vector<uint8_t> newValue = hexToBin(v, strlen(v));
  if (newValue.empty() && strlen(v) > 0)
    return false;
  setParam(newValue.data(), newValue.size());
  return true;
}

void BinaryParameter::setParam(const uint8_t* v, size_t len) {
  LOCK_CONFIG;
  if (immutable) return; 
  vlog.debug("set %s(Binary)", getName());
  delete [] value;
  value = nullptr;
  length = 0;
  if (len) {
    assert(v);
    value = new uint8_t[len];
    length = len;
    memcpy(value, v, len);
  }
}

std::string BinaryParameter::getDefaultStr() const {
  return binToHex(def_value, def_length);
}

std::string BinaryParameter::getValueStr() const {
  LOCK_CONFIG;
  return binToHex(value, length);
}

std::vector<uint8_t> BinaryParameter::getData() const {
  LOCK_CONFIG;
  std::vector<uint8_t> out(length);
  memcpy(out.data(), value, length);
  return out;
}
