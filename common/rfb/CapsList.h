/* Copyright (C) 2006 Constantin Kaplinsky. All Rights Reserved.
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
// CapsList - a list of server-side protocol capabilities. This class makes
// it easy to prepare a capability list and send it via OutStream transport.
//

#ifndef __RFB_CAPSLIST_H__
#define __RFB_CAPSLIST_H__

#include <rfb/CapsContainer.h>

namespace rdr { class OutStream; }

namespace rfb {

  // NOTE: Here we use the CapsContainer class for an off-design purpose.
  //       However, that class is so good that I believe it's better to
  //       use its well-tested code instead of writing new class from the
  //       scratch.

  class CapsList : private CapsContainer
  {
  public:

    // Constructor.
    // The maxCaps value is the maximum number of capabilities in the list.
    CapsList(int maxCaps = 64);

    virtual ~CapsList();

    // Current number of capabilities in the list.
    int getSize() const { return numEnabled(); }

    // Does the list include nothing more than one particular capability?
    bool includesOnly(rdr::U32 code) {
      return (numEnabled() == 1 && getByOrder(0) == code);
    }

    // Add capability ("standard" vendor).
    void addStandard(rdr::U32 code, const char *name);
    // Add capability (TightVNC vendor).
    void addTightExt(rdr::U32 code, const char *name);
    // Add capability (any other vendor).
    void add3rdParty(rdr::U32 code, const char *name, const char *vendor);

    // Send the list of capabilities (not including the counter).
    void write(rdr::OutStream* os) const;

  protected:

    // Pre-defined signatures for known vendors.
    static const char *const VENDOR_STD;
    static const char *const VENDOR_TIGHT;
  };

}

#endif // __RFB_CAPSLIST_H__
