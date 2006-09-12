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
// CapsList class implementation.
// Member functions are documented in CapsList.h.
//

#include <rfb/CapsList.h>
#include <rdr/OutStream.h>

using namespace rfb;

const char *const CapsList::VENDOR_STD = "STDV";
const char *const CapsList::VENDOR_TIGHT = "TGHT";

CapsList::CapsList(int maxCaps)
  : CapsContainer(maxCaps)
{
}

CapsList::~CapsList()
{
}

void
CapsList::addStandard(rdr::U32 code, const char *name)
{
  add3rdParty(code, name, VENDOR_STD);
}

void
CapsList::addTightExt(rdr::U32 code, const char *name)
{
  add3rdParty(code, name, VENDOR_TIGHT);
}

void
CapsList::add3rdParty(rdr::U32 code, const char *name, const char *vendor)
{
  add(code, vendor, name);

  // NOTE: This code is a bit tricky and not the most efficient. However,
  //       that's not a problem as we prefer simplicity to performance here.
  //       Here we need to "enable capability" but that requires comparing
  //       name and vendor strings. So we just make CapsContainer compare
  //       the same strings with themselves.
  CapabilityInfo tmp;
  if (getInfo(code, &tmp))
    enable(&tmp);
}

void
CapsList::write(rdr::OutStream* os) const
{
  int count = getSize();
  CapabilityInfo cap;

  for (int i = 0; i < count; i++) {
    getInfo(getByOrder(i), &cap);
    os->writeU32(cap.code);
    os->writeBytes(&cap.vendorSignature, 4);
    os->writeBytes(&cap.nameSignature, 8);
  }
}

