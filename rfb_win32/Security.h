/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

// Security.h

// Wrapper classes for a few Windows NT security structures/functions
// that are used by VNC

#ifndef __RFB_WIN32_SECURITY_H__
#define __RFB_WIN32_SECURITY_H__

#include <rdr/types.h>
#include <rdr/Exception.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/TCharArray.h>

#include <lmcons.h>
#include <Accctrl.h>
#include <aclapi.h>

#include <list>

namespace rfb {

  namespace win32 {

    struct Trustee : public TRUSTEE {
      Trustee(const TCHAR* name,
              TRUSTEE_FORM form=TRUSTEE_IS_NAME,
              TRUSTEE_TYPE type=TRUSTEE_IS_UNKNOWN)
      {
        pMultipleTrustee = 0;
        MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        TrusteeForm = form;
        TrusteeType = type;
        ptstrName = (TCHAR*)name;
      }
    };

    struct ExplicitAccess : public EXPLICIT_ACCESS {
      ExplicitAccess(const TCHAR* name,
                     TRUSTEE_FORM type,
                     DWORD perms,
                     ACCESS_MODE mode,
                     DWORD inherit=0)
      {
        Trustee = rfb::win32::Trustee(name, type);
        grfAccessPermissions = perms;
        grfAccessMode = mode;
        grfInheritance = inherit;
      }
    };

    // Helper class for building access control lists
    struct AccessEntries {
      AccessEntries() : entries(0), entry_count(0) {}
      ~AccessEntries() {delete [] entries;}
      void allocMinEntries(int count) {
        if (count > entry_count) {
          EXPLICIT_ACCESS* new_entries = new EXPLICIT_ACCESS[entry_count+1];
          if (entries) {
            memcpy(new_entries, entries, sizeof(EXPLICIT_ACCESS) * entry_count);
            delete entries;
          }
          entries = new_entries;
        }
      }
      void addEntry(const TCHAR* trusteeName,
                    DWORD permissions,
                    ACCESS_MODE mode)
      {
        allocMinEntries(entry_count+1);
        ZeroMemory(&entries[entry_count], sizeof(EXPLICIT_ACCESS));
        entries[entry_count] = ExplicitAccess(trusteeName, TRUSTEE_IS_NAME, permissions, mode);
        entry_count++;
      }
      void addEntry(const PSID sid, DWORD permissions, ACCESS_MODE mode) {
        allocMinEntries(entry_count+1);
        ZeroMemory(&entries[entry_count], sizeof(EXPLICIT_ACCESS));
        entries[entry_count] = ExplicitAccess((TCHAR*)sid, TRUSTEE_IS_SID, permissions, mode);
        entry_count++;
      }

      EXPLICIT_ACCESS* entries;
      int entry_count;
    };

    // Helper class for handling SIDs
    struct Sid {
      Sid() : sid(0) {}
      Sid(PSID sid_) : sid(sid_) {}
      ~Sid() {
        if (sid) FreeSid(sid);
      }
      operator PSID() const {return sid;}
      PSID operator=(const PSID sid_) {
        if (sid) FreeSid(sid);
        sid = sid_;
      }

      static PSID Administrators() {
        PSID sid = 0;
        SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
        if (!AllocateAndInitializeSid(&ntAuth, 2,
          SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
          0, 0, 0, 0, 0, 0,
          &sid)) 
          throw rdr::SystemException("Sid::Administrators", GetLastError());
        return sid;
      }
      static PSID SYSTEM() {
        PSID sid = 0;
        SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
        if (!AllocateAndInitializeSid(&ntAuth, 1,
          SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0,
          &sid))
          throw rdr::SystemException("Sid::SYSTEM", GetLastError());
        return sid;
      }

    protected:
      PSID sid;
    };
      
    // Helper class for handling & freeing ACLs
    struct AccessControlList : public LocalMem {
      AccessControlList(int size) : LocalMem(size) {}
      AccessControlList(PACL acl_=0) : LocalMem(acl_) {}
      operator PACL() {return (PACL)ptr;}
    };

    // Create a new ACL based on supplied entries and, if supplied, existing ACL 
    static PACL CreateACL(const AccessEntries& ae, PACL existing_acl=0) {
      typedef DWORD (WINAPI *_SetEntriesInAcl_proto) (ULONG, PEXPLICIT_ACCESS, PACL, PACL*);
#ifdef UNICODE
      const char* fnName = "SetEntriesInAclW";
#else
      const char* fnName = "SetEntriesInAclA";
#endif
      DynamicFn<_SetEntriesInAcl_proto> _SetEntriesInAcl(_T("advapi32.dll"), fnName);
      if (!_SetEntriesInAcl.isValid())
        throw rdr::SystemException("CreateACL failed; no SetEntriesInAcl", ERROR_CALL_NOT_IMPLEMENTED);
      PACL new_dacl;
      DWORD result;
      if ((result = (*_SetEntriesInAcl)(ae.entry_count, ae.entries, existing_acl, &new_dacl)) != ERROR_SUCCESS)
        throw rdr::SystemException("SetEntriesInAcl", result);
      return new_dacl;
    }

    // Helper class for memory-management of self-relative SecurityDescriptors
    struct SecurityDescriptorPtr : LocalMem {
      SecurityDescriptorPtr(int size) : LocalMem(size) {}
      SecurityDescriptorPtr(PSECURITY_DESCRIPTOR sd_=0) : LocalMem(sd_) {}
      PSECURITY_DESCRIPTOR takeSD() {return takePtr();}
    };

    // Create a new self-relative Security Descriptor, owned by SYSTEM/Administrators,
    //   with the supplied DACL and no SACL.  The returned value can be assigned
    //   to a SecurityDescriptorPtr to be managed.
    static PSECURITY_DESCRIPTOR CreateSdWithDacl(const PACL dacl) {
      SECURITY_DESCRIPTOR absSD;
      if (!InitializeSecurityDescriptor(&absSD, SECURITY_DESCRIPTOR_REVISION))
        throw rdr::SystemException("InitializeSecurityDescriptor", GetLastError());
      Sid owner(Sid::SYSTEM());
      if (!SetSecurityDescriptorOwner(&absSD, owner, FALSE))
        throw rdr::SystemException("SetSecurityDescriptorOwner", GetLastError());
      Sid group(Sid::Administrators());
      if (!SetSecurityDescriptorGroup(&absSD, group, FALSE))
        throw rdr::SystemException("SetSecurityDescriptorGroupp", GetLastError());
      if (!SetSecurityDescriptorDacl(&absSD, TRUE, dacl, FALSE))
        throw rdr::SystemException("SetSecurityDescriptorDacl", GetLastError());
      DWORD sdSize = GetSecurityDescriptorLength(&absSD);
      SecurityDescriptorPtr sd(sdSize);
      if (!MakeSelfRelativeSD(&absSD, sd, &sdSize))
        throw rdr::SystemException("MakeSelfRelativeSD", GetLastError());
      return sd.takeSD();
    }

  }

}

#endif
