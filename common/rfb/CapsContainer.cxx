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
// CapsContainer class implementation.
// FIXME: Extend the comment.
//

#include <rfb/CapsContainer.h>

using namespace rfb;

//
// The constructor.
//

CapsContainer::CapsContainer(int maxCaps)
: m_maxSize(maxCaps), m_listSize(0), m_plist(new rdr::U32[m_maxSize])
{
}

//
// The destructor.
//

CapsContainer::~CapsContainer()
{
  delete[] m_plist;

  // Remove char[] strings allocated by the new[] operator.
  std::map<rdr::U32,char*>::const_iterator iter;
  for (iter = m_descMap.begin(); iter != m_descMap.end(); iter++) {
    delete[] iter->second;
  }
}

//
// Add information about a particular capability into the object. There are
// two functions to perform this task. These functions overwrite capability
// records with the same code.
//

void
CapsContainer::add(rdr::U32 code, const char *vendor, const char *name,
                   const char *desc)
{
  // Fill in an rfbCapabilityInfo structure and pass it to the overloaded
  // function.
  CapabilityInfo capinfo;
  capinfo.code = code;
  memcpy(capinfo.vendorSignature, vendor, 4);
  memcpy(capinfo.nameSignature, name, 8);
  add(&capinfo, desc);
}

void
CapsContainer::add(const CapabilityInfo *capinfo, const char *desc)
{
  m_infoMap[capinfo->code] = *capinfo;
  m_enableMap[capinfo->code] = false;

  if (isKnown(capinfo->code)) {
    delete[] m_descMap[capinfo->code];
  }
  char *desc_copy = 0;
  if (desc != 0) {
    desc_copy = new char[strlen(desc) + 1];
    strcpy(desc_copy, desc);
  }
  m_descMap[capinfo->code] = desc_copy;
}

//
// Check if a capability with the specified code was added earlier.
//

bool
CapsContainer::isKnown(rdr::U32 code) const
{
  return (m_descMap.find(code) != m_descMap.end());
}

//
// Fill in a rfbCapabilityInfo structure with contents corresponding to the
// specified code. Returns true on success, false if the specified code is
// not known.
//

bool
CapsContainer::getInfo(rdr::U32 code, CapabilityInfo *capinfo) const
{
  if (isKnown(code)) {
    *capinfo = m_infoMap.find(code)->second;
    return true;
  }

  return false;
}

//
// Get a description string for the specified capability code. Returns 0
// either if the code is not known, or if there is no description for this
// capability.
//

char *
CapsContainer::getDescription(rdr::U32 code) const
{
  return (isKnown(code)) ? m_descMap.find(code)->second : 0;
}

//
// Mark the specified capability as "enabled". This function checks "vendor"
// and "name" signatures in the existing record and in the argument structure
// and enables the capability only if both records are the same.
//

bool
CapsContainer::enable(const CapabilityInfo *capinfo)
{
  if (!isKnown(capinfo->code))
    return false;

  const CapabilityInfo *known = &(m_infoMap[capinfo->code]);
  if ( memcmp(known->vendorSignature, capinfo->vendorSignature, 4) != 0 ||
       memcmp(known->nameSignature, capinfo->nameSignature, 8) != 0 ) {
    m_enableMap[capinfo->code] = false;
    return false;
  }

  m_enableMap[capinfo->code] = true;
  if (m_listSize < m_maxSize) {
    m_plist[m_listSize++] = capinfo->code;
  }
  return true;
}

//
// Check if the specified capability is known and enabled.
//

bool
CapsContainer::isEnabled(rdr::U32 code) const
{
  return (isKnown(code)) ? m_enableMap.find(code)->second : false;
}

//
// Return the capability code at the specified index.
// If the index is not valid, return 0.
//

rdr::U32
CapsContainer::getByOrder(int idx) const
{
  return (idx < m_listSize) ? m_plist[idx] : 0;
}

