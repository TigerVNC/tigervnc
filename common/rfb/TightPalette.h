/* Copyright (C) 2000-2005 Constantin Kaplinsky.  All Rights Reserved.
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
// TightPalette class is a container for ordered color values. Colors
// are keys in a hash where values are frequency counts. Also, there
// is a list where colors are always sorted by these counts (more
// frequent first).
//

#ifndef __RFB_TIGHTPALETTE_H__
#define __RFB_TIGHTPALETTE_H__

#include <string.h>
#include <rdr/types.h>

namespace rfb {

  struct TightColorList {
    TightColorList *next;
    int idx;
    rdr::U32 rgb;
  };

  struct TightPaletteEntry {
    TightColorList *listNode;
    int numPixels;
  };

  class TightPalette {

  protected:

    // FIXME: Bigger hash table? Better hash function?
    inline static int hashFunc(rdr::U32 rgb) {
      return (rgb ^ (rgb >> 13)) & 0xFF;
    }

  public:

    TightPalette(int maxColors = 254);

    //
    // Re-initialize the object. This does not change maximum number
    // of colors.
    //
    void reset();

    //
    // Set limit on the number of colors in the palette. Note that
    // this value cannot exceed 254.
    //
    void setMaxColors(int maxColors);

    //
    // Insert new color into the palette, or increment its counter if
    // the color is already there. Returns new number of colors, or
    // zero if the palette is full. If the palette becomes full, it
    // reports zero colors and cannot be used any more without calling
    // reset().
    //
    int insert(rdr::U32 rgb, int numPixels);

    //
    // Return number of colors in the palette.
    //
    inline int getNumColors() const {
      return m_numColors;
    }

    //
    // Return the color specified by its index in the palette.
    //
    inline rdr::U32 getEntry(int i) const {
      return (i < m_numColors) ? m_entry[i].listNode->rgb : (rdr::U32)-1;
    }

    //
    // Return the pixel counter of the color specified by its index.
    //
    inline int getCount(int i) const {
      return (i < m_numColors) ? m_entry[i].numPixels : 0;
    }

    //
    // Return the index of a specified color.
    //
    inline rdr::U8 getIndex(rdr::U32 rgb) const {
      TightColorList *pnode = m_hash[hashFunc(rgb)];
      while (pnode != NULL) {
        if (pnode->rgb == rgb) {
          return (rdr::U8)pnode->idx;
        }
        pnode = pnode->next;
      }
      return 0xFF;              // no such color
    }

  protected:

    int m_maxColors;
    int m_numColors;

    TightPaletteEntry m_entry[256];
    TightColorList *m_hash[256];
    TightColorList m_list[256];

  };

} // namespace rfb

#endif // __RFB_TIGHTPALETTE_H__
