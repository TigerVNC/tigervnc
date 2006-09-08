/* Copyright (C) 2003-2006 Constantin Kaplinsky. All Rights Reserved.
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

//
// CapsContainer and associated structures - dealing with TightVNC-specific
// protocol capability lists.
//

#ifndef __RFB_CAPSCONTAINER_H__
#define __RFB_CAPSCONTAINER_H__

#include <rdr/types.h>

#include <map>

namespace rfb {

  //
  // Structure used to describe protocol options such as tunneling methods,
  // authentication schemes and message types (protocol versions 3.7t/3.8t).
  //

  struct CapabilityInfo {
    rdr::U32 code;                // numeric identifier
    rdr::U8 vendorSignature[4];   // vendor identification
    rdr::U8 nameSignature[8];     // abbreviated option name
  };


  //
  // CapsContainer - a container class to maintain a list of protocol
  // capabilities.
  //

  class CapsContainer {
  public:
    CapsContainer(int maxCaps = 64);
    virtual ~CapsContainer();

    void add(const CapabilityInfo *capinfo, const char *desc = 0);
    void add(rdr::U32 code, const char *vendor, const char *name,
             const char *desc = 0);

    bool isKnown(rdr::U32 code) const;
    bool getInfo(rdr::U32 code, CapabilityInfo *capinfo) const;
    char *getDescription(rdr::U32 code) const;

    bool enable(const CapabilityInfo *capinfo);
    bool isEnabled(rdr::U32 code) const;
    int numEnabled() const { return m_listSize; }
    rdr::U32 getByOrder(int idx) const;

  private:
    std::map<rdr::U32,CapabilityInfo> m_infoMap;
    std::map<rdr::U32,char*> m_descMap;
    std::map<rdr::U32,bool> m_enableMap;

    int m_maxSize;
    int m_listSize;
    rdr::U32 *m_plist;
  };

}

#endif // __RFB_CAPSCONTAINER_H__
