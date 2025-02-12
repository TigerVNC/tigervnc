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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <core/Configuration.h>
#include <core/LogWriter.h>

#include <rfb/KeyRemapper.h>

using namespace rfb;

static core::LogWriter vlog("KeyRemapper");

KeyRemapper KeyRemapper::defInstance;

KeyRemapper::KeyRemapper()
{
}

KeyRemapper::~KeyRemapper()
{
}

void KeyRemapper::setMapping(const std::map<uint32_t,uint32_t>& m)
{
  mapping = m;
}

uint32_t KeyRemapper::remapKey(uint32_t key) const {
  std::map<uint32_t,uint32_t>::const_iterator i = mapping.find(key);
  if (i != mapping.end())
    return i->second;
  return key;
}


class KeyMapParameter : public core::StringListParameter {
public:
  KeyMapParameter()
    : core::StringListParameter("RemapKeys",
                                "Comma-separated list of incoming "
                                "keysyms to remap.  Mappings are "
                                "expressed as two hex values, prefixed "
                                "by 0x, and separated by ->",
                                {}) {
    KeyRemapper::defInstance.setMapping({});
  }
  bool setParam(const char* v) override {
    std::map<uint32_t,uint32_t> mapping;

    if (!core::StringListParameter::setParam(v))
      return false;

    for (const char* m : *this) {
      int from, to;
      char bidi;
      if (sscanf(m, "0x%x%c>0x%x", &from,
                &bidi, &to) == 3) {
        if (bidi != '-' && bidi != '<')
          vlog.error("Warning: Unknown operation %c>, assuming ->", bidi);
        mapping[from] = to;
        if (bidi == '<')
          mapping[to] = from;
      } else {
        vlog.error("Warning: Bad mapping %s", m);
      }
    }

    KeyRemapper::defInstance.setMapping(mapping);

    return true;
  }
} defaultParam;


