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
#ifndef __RFB_TRUECOLOURMAP_H__
#define __RFB_TRUECOLOURMAP_H__

#include <rfb/ColourMap.h>

namespace rfb {

  class TrueColourMap : public ColourMap {
  public:
    TrueColourMap(const PixelFormat& pf_) : pf(pf_) {}

    virtual void lookup(int i, int* r, int* g, int* b)
    {
      rdr::U16 _r, _g, _b;
      pf.rgbFromPixel(i, NULL, &_r, &_g, &_b);
      *r = _r;
      *g = _g;
      *b = _b;
    }
  private:
    PixelFormat pf;
  };
}
#endif
