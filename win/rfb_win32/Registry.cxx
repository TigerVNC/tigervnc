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

// -=- Registry.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/Registry.h>
#include <rfb_win32/Security.h>
#include <rdr/MemOutStream.h>
#include <rdr/HexOutStream.h>
#include <rdr/HexInStream.h>
#include <stdlib.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>

// These flags are required to control access control inheritance,
// but are not defined by VC6's headers.  These definitions comes
// from the Microsoft Platform SDK.
#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#endif
#ifndef UNPROTECTED_DACL_SECURITY_INFORMATION
#define UNPROTECTED_DACL_SECURITY_INFORMATION     (0x20000000L)
#endif


using namespace rfb;
using namespace rfb::win32;


static LogWriter vlog("Registry");


RegKey::RegKey() : key(nullptr), freeKey(false), valueName(nullptr), valueNameBufLen(0) {}

RegKey::RegKey(const HKEY k) : key(nullptr), freeKey(false), valueName(nullptr), valueNameBufLen(0) {
  LONG result = RegOpenKeyEx(k, nullptr, 0, KEY_ALL_ACCESS, &key);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegOpenKeyEx(HKEY)", result);
  vlog.debug("duplicated %p to %p", k, key);
  freeKey = true;
}

RegKey::RegKey(const RegKey& k) : key(nullptr), freeKey(false), valueName(nullptr), valueNameBufLen(0) {
  LONG result = RegOpenKeyEx(k.key, nullptr, 0, KEY_ALL_ACCESS, &key);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegOpenKeyEx(RegKey&)", result);
  vlog.debug("duplicated %p to %p", k.key, key);
  freeKey = true;
}

RegKey::~RegKey() {
  close();
  delete [] valueName;
}


void RegKey::setHKEY(HKEY k, bool fK) {
  vlog.debug("setHKEY(%p,%d)", k, (int)fK);
  close();
  freeKey = fK;
  key = k;
}


bool RegKey::createKey(const RegKey& root, const char* name) {
  close();
  LONG result = RegCreateKey(root.key, name, &key);
  if (result != ERROR_SUCCESS) {
    vlog.error("RegCreateKey(%p, %s): %lx", root.key, name, result);
    throw rdr::SystemException("RegCreateKeyEx", result);
  }
  vlog.debug("createKey(%p,%s) = %p", root.key, name, key);
  freeKey = true;
  return true;
}

void RegKey::openKey(const RegKey& root, const char* name, bool readOnly) {
  close();
  LONG result = RegOpenKeyEx(root.key, name, 0, readOnly ? KEY_READ : KEY_ALL_ACCESS, &key);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegOpenKeyEx (open)", result);
  vlog.debug("openKey(%p,%s,%s) = %p", root.key, name,
	         readOnly ? "ro" : "rw", key);
  freeKey = true;
}

void RegKey::setDACL(const PACL acl, bool inherit) {
  DWORD result;
  if ((result = SetSecurityInfo(key, SE_REGISTRY_KEY,
    DACL_SECURITY_INFORMATION |
    (inherit ? UNPROTECTED_DACL_SECURITY_INFORMATION : PROTECTED_DACL_SECURITY_INFORMATION),
    nullptr, nullptr, acl, nullptr)) != ERROR_SUCCESS)
    throw rdr::SystemException("RegKey::setDACL failed", result);
}

void RegKey::close() {
  if (freeKey) {
    vlog.debug("RegCloseKey(%p)", key);
    RegCloseKey(key);
    key = nullptr;
  }
}

void RegKey::deleteKey(const char* name) const {
  LONG result = RegDeleteKey(key, name);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegDeleteKey", result);
}

void RegKey::deleteValue(const char* name) const {
  LONG result = RegDeleteValue(key, name);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegDeleteValue", result);
}

void RegKey::awaitChange(bool watchSubTree, DWORD filter, HANDLE event) const {
  LONG result = RegNotifyChangeKeyValue(key, watchSubTree, filter, event, event != nullptr);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegNotifyChangeKeyValue", result);
}


RegKey::operator HKEY() const {return key;}


void RegKey::setExpandString(const char* valname, const char* value) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_EXPAND_SZ, (const BYTE*)value, (strlen(value)+1)*sizeof(char));
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setExpandString", result);
}

void RegKey::setString(const char* valname, const char* value) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_SZ, (const BYTE*)value, (strlen(value)+1)*sizeof(char));
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setString", result);
}

void RegKey::setBinary(const char* valname, const void* value, size_t length) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_BINARY, (const BYTE*)value, length);
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setBinary", result);
}

void RegKey::setInt(const char* valname, int value) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setInt", result);
}

void RegKey::setBool(const char* valname, bool value) const {
  setInt(valname, value ? 1 : 0);
}

std::string RegKey::getString(const char* valname) const {
  return getRepresentation(valname);
}

std::string RegKey::getString(const char* valname, const char* def) const {
  try {
    return getString(valname);
  } catch(rdr::Exception&) {
    return def;
  }
}

std::vector<uint8_t> RegKey::getBinary(const char* valname) const {
  std::string hex = getRepresentation(valname);
  return hexToBin(hex.data(), hex.size());
}
std::vector<uint8_t> RegKey::getBinary(const char* valname, const uint8_t* def, size_t deflen) const {
  try {
    return getBinary(valname);
  } catch(rdr::Exception&) {
    std::vector<uint8_t> out(deflen);
    memcpy(out.data(), def, deflen);
    return out;
  }
}

int RegKey::getInt(const char* valname) const {
  return atoi(getRepresentation(valname).c_str());
}
int RegKey::getInt(const char* valname, int def) const {
  try {
    return getInt(valname);
  } catch(rdr::Exception&) {
    return def;
  }
}

bool RegKey::getBool(const char* valname) const {
  return getInt(valname) > 0;
}
bool RegKey::getBool(const char* valname, bool def) const {
  return getInt(valname, def ? 1 : 0) > 0;
}

std::string RegKey::getRepresentation(const char* valname) const {
  DWORD type, length;
  LONG result = RegQueryValueEx(key, valname, nullptr, &type, nullptr, &length);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("get registry value length", result);
  std::vector<uint8_t> data(length);
  result = RegQueryValueEx(key, valname, nullptr, &type, (BYTE*)data.data(), &length);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("get registry value", result);

  switch (type) {
  case REG_BINARY:
    {
      return binToHex(data.data(), length);
    }
  case REG_SZ:
    if (length) {
      return std::string((char*)data.data(), length);
    } else {
      return "";
    }
  case REG_DWORD:
    {
      char tmp[16];
      sprintf(tmp, "%lu", *((DWORD*)data.data()));
      return tmp;
    }
  case REG_EXPAND_SZ:
    {
    if (length) {
      std::string str((char*)data.data(), length);
      DWORD required = ExpandEnvironmentStrings(str.c_str(), nullptr, 0);
      if (required==0)
        throw rdr::SystemException("ExpandEnvironmentStrings", GetLastError());
      std::vector<char> expanded(required);
      length = ExpandEnvironmentStrings(str.c_str(), expanded.data(), required);
      if (required<length)
        throw rdr::Exception("unable to expand environment strings");
      return expanded.data();
    } else {
      return "";
    }
    }
  default:
    throw rdr::Exception("unsupported registry type");
  }
}

bool RegKey::isValue(const char* valname) const {
  try {
    getRepresentation(valname);
    return true;
  } catch(rdr::Exception&) {
    return false;
  }
}

const char* RegKey::getValueName(int i) {
  DWORD maxValueNameLen;
  LONG result = RegQueryInfoKey(key, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &maxValueNameLen, nullptr, nullptr, nullptr);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegQueryInfoKey", result);
  if (valueNameBufLen < maxValueNameLen + 1) {
    valueNameBufLen = maxValueNameLen + 1;
    delete [] valueName;
    valueName = new char[valueNameBufLen];
  }
  DWORD length = valueNameBufLen;
  result = RegEnumValue(key, i, valueName, &length, nullptr, nullptr, nullptr, nullptr);
  if (result == ERROR_NO_MORE_ITEMS) return nullptr;
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegEnumValue", result);
  return valueName;
}

const char* RegKey::getKeyName(int i) {
  DWORD maxValueNameLen;
  LONG result = RegQueryInfoKey(key, nullptr, nullptr, nullptr, nullptr, &maxValueNameLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegQueryInfoKey", result);
  if (valueNameBufLen < maxValueNameLen + 1) {
    valueNameBufLen = maxValueNameLen + 1;
    delete [] valueName;
    valueName = new char[valueNameBufLen];
  }
  DWORD length = valueNameBufLen;
  result = RegEnumKeyEx(key, i, valueName, &length, nullptr, nullptr, nullptr, nullptr);
  if (result == ERROR_NO_MORE_ITEMS) return nullptr;
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegEnumKey", result);
  return valueName;
}
