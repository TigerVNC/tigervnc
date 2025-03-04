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

#include <algorithm>
#include <stdexcept>

#include <core/Configuration.h>
#include <core/LogWriter.h>
#include <core/string.h>

#include <rdr/HexOutStream.h>
#include <rdr/HexInStream.h>

using namespace core;

static LogWriter vlog("Config");


// -=- The Global Configuration object
Configuration* Configuration::global_ = nullptr;

Configuration* Configuration::global() {
  if (!global_)
    global_ = new Configuration();
  return global_;
}

// -=- Configuration implementation

bool Configuration::set(const char* paramName, const char* val,
                        bool immutable)
{
  for (VoidParameter* current: params) {
    if (strcasecmp(current->getName(), paramName) == 0) {
      bool b = current->setParam(val);
      if (b && immutable) 
	current->setImmutable();
      return b;
    }
  }
  return false;
}

VoidParameter* Configuration::get(const char* param)
{
  for (VoidParameter* current: params) {
    if (strcasecmp(current->getName(), param) == 0)
      return current;
  }
  return nullptr;
}

void Configuration::list(int width, int nameWidth) {
  for (VoidParameter* current: params) {
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
  }
}


bool Configuration::remove(const char* param) {
  std::list<VoidParameter*>::iterator iter;

  iter = std::find_if(params.begin(), params.end(),
                      [param](VoidParameter* p) {
                        return strcasecmp(p->getName(), param) == 0;
                      });
  if (iter == params.end())
    return false;

  params.erase(iter);
  return true;
}

int Configuration::handleArg(int argc, char* argv[], int index)
{
  std::string param, val;
  const char* equal = strchr(argv[index], '=');

  if (equal == argv[index])
    return 0;

  if (equal) {
    param.assign(argv[index], equal-argv[index]);
    val.assign(equal+1);
  } else {
    param.assign(argv[index]);
  }

  if ((param.length() > 0) && (param[0] == '-')) {
    // allow gnu-style --<option>
    if ((param.length() > 1) && (param[1] == '-'))
      param = param.substr(2);
    else
      param = param.substr(1);
  } else {
    // All command line arguments need either an initial '-', or an '='
    if (!equal)
      return 0;
  }

  if (equal)
    return set(param.c_str(), val.c_str()) ? 1 : 0;

  for (VoidParameter* current: params) {
    if (strcasecmp(current->getName(), param.c_str()) != 0)
      continue;

    // We need to resolve an ambiguity for booleans
    if (dynamic_cast<BoolParameter*>(current) != nullptr) {
      if (index+1 < argc) {
        // FIXME: Should not duplicate the list of values here
        if ((strcasecmp(argv[index+1], "0") == 0) ||
            (strcasecmp(argv[index+1], "1") == 0) ||
            (strcasecmp(argv[index+1], "on") == 0) ||
            (strcasecmp(argv[index+1], "off") == 0) ||
            (strcasecmp(argv[index+1], "true") == 0) ||
            (strcasecmp(argv[index+1], "false") == 0) ||
            (strcasecmp(argv[index+1], "yes") == 0) ||
            (strcasecmp(argv[index+1], "no") == 0)) {
            return current->setParam(argv[index+1]) ? 2 : 0;
        }
      }
    }

    if (current->setParam())
      return 1;

    if (index+1 >= argc)
      return 0;

    return current->setParam(argv[index+1]) ? 2 : 0;
  }

  return 0;
}


// -=- VoidParameter

VoidParameter::VoidParameter(const char* name_, const char* desc_)
  : immutable(false), name(name_), description(desc_)
{
  Configuration *conf;

  conf = Configuration::global();
  conf->params.push_back(this);
  conf->params.sort([](const VoidParameter* a, const VoidParameter* b) {
    return strcasecmp(a->getName(), b->getName()) < 0;
  });
}

VoidParameter::~VoidParameter() {
  Configuration *conf;

  conf = Configuration::global();
  conf->params.remove(this);
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

bool VoidParameter::isDefault() const {
  return getDefaultStr() == getValueStr();
}

void
VoidParameter::setImmutable() {
  vlog.debug("Set immutable %s", getName());
  immutable = true;
}

// -=- AliasParameter

AliasParameter::AliasParameter(const char* name_, const char* desc_,
                               VoidParameter* param_)
  : VoidParameter(name_, desc_), param(param_) {
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

void
AliasParameter::setImmutable() {
  vlog.debug("Set immutable %s (Alias)", getName());
  param->setImmutable();
}


// -=- BoolParameter

BoolParameter::BoolParameter(const char* name_, const char* desc_, bool v)
: VoidParameter(name_, desc_), value(v), def_value(v) {
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
    vlog.error("Bool parameter %s: Invalid value '%s'", getName(), v);
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
  vlog.debug("Set %s(Bool) to %s", getName(), getValueStr().c_str());
}

std::string BoolParameter::getDefaultStr() const {
  return def_value ? "on" : "off";
}

std::string BoolParameter::getValueStr() const {
  return value ? "on" : "off";
}

BoolParameter::operator bool() const {
  return value;
}

// -=- IntParameter

IntParameter::IntParameter(const char* name_, const char* desc_, int v,
                           int minValue_, int maxValue_)
  : VoidParameter(name_, desc_), value(v), def_value(v),
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
  vlog.debug("Set %s(Int) to %d", getName(), v);
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
                                 const char* v)
  : VoidParameter(name_, desc_), value(v?v:""), def_value(v?v:"")
{
  if (!v) {
    vlog.error("Default value <null> for %s not allowed",name_);
    throw std::invalid_argument("Default value <null> not allowed");
  }
}

bool StringParameter::setParam(const char* v) {
  if (immutable) return true;
  if (!v)
    throw std::invalid_argument("setParam(<null>) not allowed");
  vlog.debug("Set %s(String) to %s", getName(), v);
  value = v;
  return true;
}

std::string StringParameter::getDefaultStr() const {
  return def_value;
}

std::string StringParameter::getValueStr() const {
  return value;
}

StringParameter::operator const char *() const {
  return value.c_str();
}

// -=- BinaryParameter

BinaryParameter::BinaryParameter(const char* name_, const char* desc_,
				 const uint8_t* v, size_t l)
: VoidParameter(name_, desc_),
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
  if (immutable) return; 
  vlog.debug("Set %s(Binary)", getName());
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
  return binToHex(value, length);
}

std::vector<uint8_t> BinaryParameter::getData() const {
  std::vector<uint8_t> out(length);
  memcpy(out.data(), value, length);
  return out;
}
