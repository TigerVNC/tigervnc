/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
 * Copyright 2011-2025 Pierre Ossman for Cendio AB
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
    std::string desc_str = current->getDescription();
    if (!def_str.empty())
      desc_str += " (default=" + def_str + ")";
    const char* desc = desc_str.c_str();
    fprintf(stderr,"  %-*s -", nameWidth, current->getName());
    int column = strlen(current->getName());
    if (column < nameWidth) column = nameWidth;
    column += 4;
    while (true) {
      if (desc[0] == '\0')
        break;

      int wordLen = strcspn(desc, " \f\n\r\t\v,");
      if (wordLen == 0) {
        desc++;
        continue;
      }

      if (desc[wordLen] == ',')
        wordLen++;

      if (column + wordLen + 1 > width) {
        fprintf(stderr,"\n%*s",nameWidth+4,"");
        column = nameWidth+4;
      }
      fprintf(stderr, " ");
      column++;

      fprintf(stderr, "%.*s", wordLen, desc);
      column += wordLen;
      desc += wordLen;
    }
    fprintf(stderr,"\n");
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
  if (v < minValue || v > maxValue) {
    vlog.error("Invalid default value %d for %s", v, getName());
    throw std::invalid_argument("Invalid default value");
  }
}

bool
IntParameter::setParam(const char* v) {
  char* end;
  long n;
  if (immutable) return true;
  n = strtol(v, &end, 0);
  if ((*end != 0) || (n < INT_MIN) || (n > INT_MAX)) {
    vlog.error("Int parameter %s: Invalid value '%s'", getName(), v);
    return false;
  }
  return setParam(n);
}

bool
IntParameter::setParam(int v) {
  if (immutable) return true;
  if (v < minValue || v > maxValue) {
    vlog.error("Int parameter %s: Invalid value '%d'", getName(), v);
    return false;
  }
  vlog.debug("Set %s(Int) to %d", getName(), v);
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

// -=- EnumParameter

EnumParameter::EnumParameter(const char* name_, const char* desc_,
                             const std::set<const char*>& enums_,
                             const char* v)
  : VoidParameter(name_, desc_), value(v?v:""), def_value(v?v:"")
{
  if (!v) {
    vlog.error("Default value <null> for %s not allowed", name_);
    throw std::invalid_argument("Default value <null> not allowed");
  }

  for (const char* e: enums_) {
    if (!e) {
      vlog.error("Enumeration <null> for %s not allowed", name_);
      throw std::invalid_argument("Enumeration <null> not allowed");
    }
    enums.insert(e);
  }

  if (std::find(enums.begin(), enums.end(), def_value) == enums.end()) {
    vlog.error("Default value %s for %s is not in list of valid values",
               def_value.c_str(), name_);
    throw std::invalid_argument("Default value is not in list of valid values");
  }
}

bool EnumParameter::setParam(const char* v)
{
  std::set<std::string>::const_iterator iter;
  if (immutable) return true;
  if (!v)
    throw std::invalid_argument("setParam(<null>) not allowed");
  iter = std::find_if(enums.begin(), enums.end(),
                      [v](const std::string& e) {
                        return strcasecmp(e.c_str(), v) == 0;
                      });
  if (iter == enums.end()) {
    vlog.error("Enum parameter %s: Invalid value '%s'", getName(), v);
    return false;
  }
  vlog.debug("Set %s(Enum) to %s", getName(), iter->c_str());
  value = *iter;
  return true;
}

std::string EnumParameter::getDefaultStr() const
{
  return def_value;
}

std::string EnumParameter::getValueStr() const
{
  return value;
}

bool EnumParameter::operator==(const char* other) const
{
  return strcasecmp(value.c_str(), other) == 0;
}

bool EnumParameter::operator==(const std::string& other) const
{
  return *this == other.c_str();
}

bool EnumParameter::operator!=(const char* other) const
{
  return strcasecmp(value.c_str(), other) != 0;
}

bool EnumParameter::operator!=(const std::string& other) const
{
  return *this != other.c_str();
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

// -=- ListParameter template

template<typename ValueType>
ListParameter<ValueType>::ListParameter(const char* name_,
                                        const char* desc_,
                                        const ListType& v)
  : VoidParameter(name_, desc_), value(v), def_value(v)
{
}

template<typename ValueType>
bool ListParameter<ValueType>::setParam(const char* v)
{
  std::vector<std::string> entries;
  ListType new_value;

  if (immutable)
    return true;

  // setParam({}) ends up as setParam(nullptr)
  if (v != nullptr)
    entries = split(v, ',');

  for (std::string& entry : entries) {
    ValueType e;

    entry.erase(0, entry.find_first_not_of(" \f\n\r\t\v"));
    entry.erase(entry.find_last_not_of(" \f\n\r\t\v")+1);

    // Special case, entire v was just whitespace
    if (entry.empty() && (entries.size() == 1))
      break;

    if (!decodeEntry(entry.c_str(), &e)) {
      vlog.error("List parameter %s: Invalid value '%s'",
                 getName(), entry.c_str());
      return false;
    }

    new_value.push_back(e);
  }

  return setParam(new_value);
}

template<typename ValueType>
bool ListParameter<ValueType>::setParam(const ListType& v)
{
  ListType vnorm;
  if (immutable)
    return true;
  for (const ValueType& entry : v) {
    if (!validateEntry(entry)) {
      vlog.error("List parameter %s: Invalid value '%s'", getName(),
                 encodeEntry(entry).c_str());
      return false;
    }
    vnorm.push_back(normaliseEntry(entry));
  }
  value = vnorm;
  vlog.debug("set %s(List) to %s", getName(), getValueStr().c_str());
  return true;
}

template<typename ValueType>
std::string ListParameter<ValueType>::getDefaultStr() const
{
  std::string result;

  for (ValueType entry : def_value) {
    // FIXME: Might want to add a space here as well for readability,
    //        but this would sacrifice backward compatibility
    if (!result.empty())
      result += ',';
    result += encodeEntry(entry);
  }

  return result;
}

template<typename ValueType>
std::string ListParameter<ValueType>::getValueStr() const
{
  std::string result;

  for (ValueType entry : value) {
    // FIXME: Might want to add a space here as well for readability,
    //        but this would sacrifice backward compatibility
    if (!result.empty())
      result += ',';
    result += encodeEntry(entry);
  }

  return result;
}

template<typename ValueType>
typename ListParameter<ValueType>::const_iterator ListParameter<ValueType>::begin() const
{
  return value.begin();
}

template<typename ValueType>
typename ListParameter<ValueType>::const_iterator ListParameter<ValueType>::end() const
{
  return value.end();
}

template<typename ValueType>
bool ListParameter<ValueType>::validateEntry(const ValueType& /*entry*/) const
{
  return true;
}

template<typename ValueType>
ValueType ListParameter<ValueType>::normaliseEntry(const ValueType& entry) const
{
  return entry;
}

// -=- IntListParameter

template class core::ListParameter<int>;

IntListParameter::IntListParameter(const char* name_, const char* desc_,
                                   const ListType& v,
                                   int minValue_, int maxValue_)
  : ListParameter<int>(name_, desc_, v),
    minValue(minValue_), maxValue(maxValue_)
{
  for (int entry : v) {
    if (!validateEntry(entry)) {
      vlog.error("Invalid default value %d for %s", entry, getName());
      throw std::invalid_argument("Invalid default value");
    }
  }
}

bool IntListParameter::decodeEntry(const char* entry, int* out) const
{
  long n;
  char *end;

  assert(entry);
  assert(out);

  if (entry[0] == '\0')
    return false;

  n = strtol(entry, &end, 0);
  if ((*end != 0) || (n < INT_MIN) || (n > INT_MAX))
    return false;

  *out = n;

  return true;
}

std::string IntListParameter::encodeEntry(const int& entry) const
{
  char valstr[16];
  sprintf(valstr, "%d", entry);
  return valstr;
}

bool IntListParameter::validateEntry(const int& entry) const
{
  return (entry >= minValue) && (entry <= maxValue);
}

// -=- StringListParameter

template class core::ListParameter<std::string>;

StringListParameter::StringListParameter(const char* name_,
                                         const char* desc_,
                                         const std::list<const char*>& v_)
  : ListParameter<std::string>(name_, desc_, {})
{
  for (const char* v: v_) {
    if (!v) {
      vlog.error("Default value <null> for %s not allowed", name_);
      throw std::invalid_argument("Default value <null> not allowed");
    }
    value.push_back(v);
    def_value.push_back(v);
  }
}

StringListParameter::const_iterator StringListParameter::begin() const
{
  return ListParameter<std::string>::begin();
}

StringListParameter::const_iterator StringListParameter::end() const
{
  return ListParameter<std::string>::end();
}

bool StringListParameter::decodeEntry(const char* entry, std::string* out) const
{
  *out = entry;
  return true;
}

std::string StringListParameter::encodeEntry(const std::string& entry) const
{
  return entry;
}

// -=- EnumListEntry

EnumListEntry::EnumListEntry(const std::string& v)
  : value(v)
{
}

std::string EnumListEntry::getValueStr() const
{
  return value;
}

bool EnumListEntry::operator==(const char* other) const
{
  return strcasecmp(value.c_str(), other) == 0;
}

bool EnumListEntry::operator==(const std::string& other) const
{
  return *this == other.c_str();
}

bool EnumListEntry::operator!=(const char* other) const
{
  return strcasecmp(value.c_str(), other) != 0;
}

bool EnumListEntry::operator!=(const std::string& other) const
{
  return *this != other.c_str();
}

// -=- EnumListParameter

EnumListParameter::EnumListParameter(const char* name_,
                                     const char* desc_,
                                     const std::set<const char*>& enums_,
                                     const std::list<const char*>& v_)
  : ListParameter<std::string>(name_, desc_, {})
{
  for (const char* v: v_) {
    if (!v) {
      vlog.error("Default value <null> for %s not allowed", name_);
      throw std::invalid_argument("Default value <null> not allowed");
    }
    value.push_back(v);
    def_value.push_back(v);
  }

  for (const char* e: enums_) {
    if (!e) {
      vlog.error("Enumeration <null> for %s not allowed", name_);
      throw std::invalid_argument("Enumeration <null> not allowed");
    }
    enums.insert(e);
  }

  for (const std::string& def_entry : def_value) {
    if (std::find(enums.begin(), enums.end(), def_entry) == enums.end()) {
      vlog.error("Default value %s for %s is not in list of valid values",
                 def_entry.c_str(), name_);
      throw std::invalid_argument("Default value is not in list of valid values");
    }
  }
}

EnumListParameter::const_iterator EnumListParameter::begin() const
{
  return ListParameter<std::string>::begin();
}

EnumListParameter::const_iterator EnumListParameter::end() const
{
  return ListParameter<std::string>::end();
}

bool EnumListParameter::decodeEntry(const char* entry, std::string* out) const
{
  *out = entry;
  return true;
}

std::string EnumListParameter::encodeEntry(const std::string& entry) const
{
  return entry;
}

bool EnumListParameter::validateEntry(const std::string& entry) const
{
  for (const std::string& e : enums) {
    if (strcasecmp(e.c_str(), entry.c_str()) == 0)
      return true;
  }
  return false;
}

std::string EnumListParameter::normaliseEntry(const std::string& entry) const
{
  for (const std::string& e : enums) {
    if (strcasecmp(e.c_str(), entry.c_str()) == 0)
      return e;
  }
  throw std::logic_error("Entry is not in list of valid values");
}
