/* Copyright 2019 Pierre Ossman for Cendio AB
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
#ifndef __RFB_CLIPBOARDTYPES_H__
#define __RFB_CLIPBOARDTYPES_H__

namespace rfb {

  // Formats
  const unsigned int clipboardUTF8 = 1 << 0;
  const unsigned int clipboardRTF = 1 << 1;
  const unsigned int clipboardHTML = 1 << 2;
  const unsigned int clipboardDIB = 1 << 3;
  const unsigned int clipboardFiles = 1 << 4;

  const unsigned int clipboardFormatMask = 0x0000ffff;

  // Actions
  const unsigned int clipboardCaps = 1 << 24;
  const unsigned int clipboardRequest = 1 << 25;
  const unsigned int clipboardPeek = 1 << 26;
  const unsigned int clipboardNotify = 1 << 27;
  const unsigned int clipboardProvide = 1 << 28;

  const unsigned int clipboardActionMask = 0xff000000;
}
#endif
