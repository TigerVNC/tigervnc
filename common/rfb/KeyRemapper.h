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

#ifndef __RFB_KEYREMAPPER_H__
#define __RFB_KEYREMAPPER_H__

#include <map>

#include <stdint.h>

namespace core { class Mutex; }

namespace rfb {

  class KeyRemapper {
  public:
    KeyRemapper(const char* m="");
    ~KeyRemapper();
    void setMapping(const char* m);
    uint32_t remapKey(uint32_t key) const;
    static KeyRemapper defInstance;
  private:
    std::map<uint32_t,uint32_t> mapping;
    core::Mutex* mutex;
  };

};

#endif // __RFB_KEYREMAPPER_H__
