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

// -=- Security.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/Security.h>
#include <rfb/LogWriter.h>

#include <lmcons.h>
#include <accctrl.h>
#include <list>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SecurityWin32");


Trustee::Trustee(const char* name,
                 TRUSTEE_FORM form,
                 TRUSTEE_TYPE type) {
  pMultipleTrustee = nullptr;
  MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  TrusteeForm = form;
  TrusteeType = type;
  ptstrName = (char*)name;
}


ExplicitAccess::ExplicitAccess(const char* name,
                               TRUSTEE_FORM type,
                               DWORD perms,
                               ACCESS_MODE mode,
                               DWORD inherit) {
  Trustee = rfb::win32::Trustee(name, type);
  grfAccessPermissions = perms;
  grfAccessMode = mode;
  grfInheritance = inherit;
}


AccessEntries::AccessEntries() : entries(nullptr), entry_count(0) {}

AccessEntries::~AccessEntries() {
  delete [] entries;
}

void AccessEntries::allocMinEntries(int count) {
  if (count > entry_count) {
    EXPLICIT_ACCESS* new_entries = new EXPLICIT_ACCESS[entry_count+1];
    if (entries) {
      memcpy(new_entries, entries, sizeof(EXPLICIT_ACCESS) * entry_count);
      delete entries;
    }
    entries = new_entries;
  }
}

void AccessEntries::addEntry(const char* trusteeName,
                             DWORD permissions,
                             ACCESS_MODE mode) {
  allocMinEntries(entry_count+1);
  ZeroMemory(&entries[entry_count], sizeof(EXPLICIT_ACCESS));
  entries[entry_count] = ExplicitAccess(trusteeName, TRUSTEE_IS_NAME, permissions, mode);
  entry_count++;
}

void AccessEntries::addEntry(const PSID sid,
                             DWORD permissions,
                             ACCESS_MODE mode) {
  allocMinEntries(entry_count+1);
  ZeroMemory(&entries[entry_count], sizeof(EXPLICIT_ACCESS));
  entries[entry_count] = ExplicitAccess((char*)sid, TRUSTEE_IS_SID, permissions, mode);
  entry_count++;
}


PSID Sid::copySID(const PSID sid) {
  if (!IsValidSid(sid))
    throw std::invalid_argument("invalid SID in copyPSID");
  PSID buf = (PSID)new uint8_t[GetLengthSid(sid)];
  if (!CopySid(GetLengthSid(sid), buf, sid))
    throw rdr::Win32Exception("CopySid failed", GetLastError());
  return buf;
}

void Sid::setSID(const PSID sid) {
  if (!IsValidSid(sid))
    throw std::invalid_argument("invalid SID in copyPSID");
  resize(GetLengthSid(sid));
  if (!CopySid(GetLengthSid(sid), data(), sid))
    throw rdr::Win32Exception("CopySid failed", GetLastError());
}

void Sid::getUserNameAndDomain(char** name, char** domain) {
  DWORD nameLen = 0;
  DWORD domainLen = 0;
  SID_NAME_USE use;
  LookupAccountSid(nullptr, (PSID)*this, nullptr, &nameLen, nullptr, &domainLen, &use);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    throw rdr::Win32Exception("Unable to determine SID name lengths", GetLastError());
  vlog.info("nameLen=%lu, domainLen=%lu, use=%d", nameLen, domainLen, use);
  *name = new char[nameLen];
  *domain = new char[domainLen];
  if (!LookupAccountSid(nullptr, (PSID)*this, *name, &nameLen, *domain, &domainLen, &use))
    throw rdr::Win32Exception("Unable to lookup account SID", GetLastError());
}


Sid::Administrators::Administrators() {
  PSID sid = nullptr;
  SID_IDENTIFIER_AUTHORITY ntAuth = { SECURITY_NT_AUTHORITY };
  if (!AllocateAndInitializeSid(&ntAuth, 2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &sid)) 
    throw rdr::Win32Exception("Sid::Administrators", GetLastError());
  setSID(sid);
  FreeSid(sid);
}

Sid::SYSTEM::SYSTEM() {
  PSID sid = nullptr;
  SID_IDENTIFIER_AUTHORITY ntAuth = { SECURITY_NT_AUTHORITY };
  if (!AllocateAndInitializeSid(&ntAuth, 1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0, &sid))
          throw rdr::Win32Exception("Sid::SYSTEM", GetLastError());
  setSID(sid);
  FreeSid(sid);
}

Sid::FromToken::FromToken(HANDLE h) {
  DWORD required = 0;
  GetTokenInformation(h, TokenUser, nullptr, 0, &required);
  std::vector<uint8_t> tmp(required);
  if (!GetTokenInformation(h, TokenUser, tmp.data(), tmp.size(), &required))
    throw rdr::Win32Exception("GetTokenInformation", GetLastError());
  TOKEN_USER* tokenUser = (TOKEN_USER*)tmp.data();
  setSID(tokenUser->User.Sid);
}


PACL rfb::win32::CreateACL(const AccessEntries& ae, PACL existing_acl) {
  PACL new_dacl;
  DWORD result;
  if ((result = SetEntriesInAcl(ae.entry_count, ae.entries, existing_acl, &new_dacl)) != ERROR_SUCCESS)
    throw rdr::Win32Exception("SetEntriesInAcl", result);
  return new_dacl;
}


PSECURITY_DESCRIPTOR rfb::win32::CreateSdWithDacl(const PACL dacl) {
  SECURITY_DESCRIPTOR absSD;
  if (!InitializeSecurityDescriptor(&absSD, SECURITY_DESCRIPTOR_REVISION))
    throw rdr::Win32Exception("InitializeSecurityDescriptor", GetLastError());
  Sid::SYSTEM owner;
  if (!SetSecurityDescriptorOwner(&absSD, owner, FALSE))
    throw rdr::Win32Exception("SetSecurityDescriptorOwner", GetLastError());
  Sid::Administrators group;
  if (!SetSecurityDescriptorGroup(&absSD, group, FALSE))
    throw rdr::Win32Exception("SetSecurityDescriptorGroupp", GetLastError());
  if (!SetSecurityDescriptorDacl(&absSD, TRUE, dacl, FALSE))
    throw rdr::Win32Exception("SetSecurityDescriptorDacl", GetLastError());
  DWORD sdSize = GetSecurityDescriptorLength(&absSD);
  SecurityDescriptorPtr sd(sdSize);
  if (!MakeSelfRelativeSD(&absSD, (PSECURITY_DESCRIPTOR)sd.ptr, &sdSize))
    throw rdr::Win32Exception("MakeSelfRelativeSD", GetLastError());
  return sd.takeSD();
}
