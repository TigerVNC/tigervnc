/* Copyright (C) 2000-2005 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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
#ifndef __RFB_PALETTE_H__
#define __RFB_PALETTE_H__

#include <assert.h>
#include <string.h>

#include <rdr/types.h>

namespace rfb {
  class Palette {
  public:
    Palette() { clear(); }
    ~Palette() {}

    int size() const { return numColours; }

    void clear() { numColours = 0; memset(hash, 0, sizeof(hash)); }

    inline bool insert(rdr::U32 colour, int numPixels);
    inline unsigned char lookup(rdr::U32 colour) const;
    inline rdr::U32 getColour(unsigned char index) const;
    inline int getCount(unsigned char index) const;

  protected:
    inline unsigned char genHash(rdr::U32 colour) const;

  protected:
    int numColours;

    struct PaletteListNode {
      PaletteListNode *next;
      unsigned char idx;
      rdr::U32 colour;
    };

    struct PaletteEntry {
      PaletteListNode *listNode;
      int numPixels;
    };

    // This is the raw list of colours, allocated from 0 and up
    PaletteListNode list[256];
    // Hash table for quick lookup into the list above
    PaletteListNode *hash[256];
    // Occurances of each colour, where the 0:th entry is the most common.
    // Indices also refer to this array.
    PaletteEntry entry[256];
  };
}

inline bool rfb::Palette::insert(rdr::U32 colour, int numPixels)
{
  PaletteListNode* pnode;
  PaletteListNode* prev_pnode;
  unsigned char hash_key, idx;

  hash_key = genHash(colour);

  pnode = hash[hash_key];
  prev_pnode = NULL;

  // Do we already have an entry for this colour?
  while (pnode != NULL) {
    if (pnode->colour == colour) {
      // Yup

      idx = pnode->idx;
      numPixels = entry[idx].numPixels + numPixels;

      // The extra pixels might mean we have to adjust the sort list
      while (idx > 0) {
        if (entry[idx-1].numPixels >= numPixels)
          break;
        entry[idx] = entry[idx-1];
        entry[idx].listNode->idx = idx;
        idx--;
      }

      if (idx != pnode->idx) {
        entry[idx].listNode = pnode;
        pnode->idx = idx;
      }

      entry[idx].numPixels = numPixels;

      return true;
    }

    prev_pnode = pnode;
    pnode = pnode->next;
  }

  // Check if palette is full.
  if (numColours == 256)
    return false;

  // Create a new colour entry
  pnode = &list[numColours];
  pnode->next = NULL;
  pnode->idx = 0;
  pnode->colour = colour;

  // Add it to the hash table
  if (prev_pnode != NULL)
    prev_pnode->next = pnode;
  else
    hash[hash_key] = pnode;

  // Move palette entries with lesser pixel counts.
  idx = numColours;
  while (idx > 0) {
    if (entry[idx-1].numPixels >= numPixels)
      break;
    entry[idx] = entry[idx-1];
    entry[idx].listNode->idx = idx;
    idx--;
  }

  // And add it into the freed slot.
  pnode->idx = idx;
  entry[idx].listNode = pnode;
  entry[idx].numPixels = numPixels;

  numColours++;

  return true;
}

inline unsigned char rfb::Palette::lookup(rdr::U32 colour) const
{
  unsigned char hash_key;
  PaletteListNode* pnode;

  hash_key = genHash(colour);
  pnode = hash[hash_key];

  while (pnode != NULL) {
    if (pnode->colour == colour)
      return pnode->idx;
    pnode = pnode->next;
  }

  // We are being fed a bad colour
  assert(false);

  return 0;
}

inline rdr::U32 rfb::Palette::getColour(unsigned char index) const
{
  return entry[index].listNode->colour;
}

inline int rfb::Palette::getCount(unsigned char index) const
{
  return entry[index].numPixels;
}

inline unsigned char rfb::Palette::genHash(rdr::U32 colour) const
{
  unsigned char hash_key;

  // djb2 hash function
  hash_key = 5; // 5381 & 0xff
  for (int i = 0; i < 32; i += 8)
    hash_key = ((hash_key << 5) + hash_key) ^ (colour >> i);

  return hash_key;
}

#endif
