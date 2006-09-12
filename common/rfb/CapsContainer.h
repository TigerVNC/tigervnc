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
  // CapabilityInfo - structure used to describe protocol options such as
  // tunneling methods, authentication schemes, message types and encoding
  // types (protocol versions 3.7 and 3.8 with TightVNC extensions).
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
  // Typical usage is as follows. First, the client creates an instance
  // of the CapsContainer class for each type of capabilities (e.g.
  // authentication methods). It adds information about capabilities it
  // supports, by calling add() functions. Then, the client receives
  // information about supported capabilities from the server, and tries
  // to "enable" each capability advertised by the server. Particular
  // capability becomes enabled if it is known (added by the client) and
  // matches that one received from the server. Finally, the client can
  // check if a given capability is enabled, and also get the list of
  // capabilities in the order they were listed by the server.
  //

  class CapsContainer
  {
  public:

    // Constructor. The maxCaps argument is the maximum number of records
    // in the list used by getByOrder() function. Remaining functions do not
    // impose limitations on the number of capabilities.
    CapsContainer(int maxCaps = 64);

    // Destructor.
    virtual ~CapsContainer();

    // Add information about a particular capability into the object. These
    // functions overwrite existing capability records with the same code.
    // NOTE: Value 0 should not be used for capability codes.
    void add(const CapabilityInfo *capinfo, const char *desc = 0);
    void add(rdr::U32 code, const char *vendor, const char *name,
             const char *desc = 0);

    // Check if a capability with the specified code was added earlier.
    bool isKnown(rdr::U32 code) const;

    // Fill in an rfbCapabilityInfo structure with contents corresponding to
    // the specified code. Returns true on success, false if the specified
    // code is not known.
    bool getInfo(rdr::U32 code, CapabilityInfo *capinfo) const;

    // Get an optional description string for the specified capability code.
    // Returns 0 either if the code is not known, or if there is no
    // description for the given capability. Otherwise, the return value
    // is a pointer valid until either add() is called again for the same
    // capability, or the CapsContaner object is destroyed.
    char *getDescription(rdr::U32 code) const;

    // Mark the specified capability as "enabled". This function compares
    // "vendor" and "name" signatures in the existing record and in the
    // argument structure and enables the capability only if both records
    // are the same.
    bool enable(const CapabilityInfo *capinfo);

    // Check if the specified capability is known and enabled.
    bool isEnabled(rdr::U32 code) const;

    // Return the number of enabled capabilities.
    int numEnabled() const { return m_listSize; }

    // Return the capability code at the specified index, from the list of
    // enabled capabilities. Capabilities are indexed in the order they were
    // enabled, index 0 points to the capability which was enabled first.
    // If the index is not valid, this function returns 0.
    rdr::U32 getByOrder(int idx) const;

  private:

    // Mapping codes to corresponding CapabilityInfo structures.
    std::map<rdr::U32,CapabilityInfo> m_infoMap;
    // Mapping capability codes to corresponding descriptions.
    std::map<rdr::U32,char*> m_descMap;
    // Mapping codes to boolean flags, true for enabled capabilities.
    std::map<rdr::U32,bool> m_enableMap;

    // Allocated size of m_plist[].
    int m_maxSize;
    // Number of valid elements in m_plist[].
    int m_listSize;
    // Array of enabled capabilities (allocated in constructor).
    rdr::U32 *m_plist;
  };

}

#endif // __RFB_CAPSCONTAINER_H__
