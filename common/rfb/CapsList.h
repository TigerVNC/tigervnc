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
// CapsList class declaration.
// FIXME: Extend the comment.
//

#ifndef __RFB_CAPSLIST_H__
#define __RFB_CAPSLIST_H__

#include <rfb/CapsContainer.h>

namespace rdr { class OutStream; }

namespace rfb {

  // NOTE: Here we use the CapsContainer class for an off-design purpose.
  //       However, that class is so good that I believe it's better to
  //       use its well-tested reliable code instead of writing new class
  //       from the scratch.

  class CapsList : private CapsContainer {
  public:
    CapsList(int maxCaps = 64);
    virtual ~CapsList();

    int getSize() const { return numEnabled(); }

    void addStandard(rdr::U32 code, const char *name);
    void addTightExt(rdr::U32 code, const char *name);
    void add3rdParty(rdr::U32 code, const char *name, const char *vendor);

    void write(rdr::OutStream* os) const;

  protected:
    static const char *const VENDOR_STD;
    static const char *const VENDOR_TIGHT;
  };

}

#endif // __RFB_CAPSLIST_H__
