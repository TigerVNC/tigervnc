/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- PixelFormatList class

#include <rfbplayer/PixelFormatList.h>

PixelFormatList::PixelFormatList() {
  PixelFormatListElement PFElem;
  
  // -=- Add the default pixel formats to list
  // PF_BGR233
  PFElem.setName("8-bits depth (BGR233)");
  PFElem.setPF(&PixelFormat(8,8,0,1,7,7,3,0,3,6));
  PFList.push_back(PFElem);
  // PF_RGB555
  PFElem.setName("15-bits depth (RGB555)");
  PFElem.setPF(&PixelFormat(16,15,0,1,31,31,31,10,5,0));
  PFList.push_back(PFElem);
  // PF_BGR555
  PFElem.setName("15-bits depth (BGR555)");
  PFElem.setPF(&PixelFormat(16,15,0,1,31,31,31,0,5,10));
  PFList.push_back(PFElem);
  // PF_RGB565
  PFElem.setName("16-bits depth (RGB565)");
  PFElem.setPF(&PixelFormat(16,16,0,1,31,63,31,11,5,0));
  PFList.push_back(PFElem);
  // PF_BGR565
  PFElem.setName("16-bits depth (BGR565)");
  PFElem.setPF(&PixelFormat(16,16,0,1,31,63,31,0,5,11));
  PFList.push_back(PFElem);
  // PF_RGB888
  PFElem.setName("24-bits depth (RGB888)");
  PFElem.setPF(&PixelFormat(32,24,0,1,255,255,255,16,8,0));
  PFList.push_back(PFElem);
  // PF_BGR888
  PFElem.setName("24-bits depth (BGR888)");
  PFElem.setPF(&PixelFormat(32,24,0,1,255,255,255,0,8,16));
  PFList.push_back(PFElem);
}

PixelFormatListElement PixelFormatList::operator[](int index) {
  return *getIterator(index);
}

void PixelFormatList::add(char *format_name, PixelFormat PF) {
  PixelFormatListElement PFElem;
  PFElem.setName(format_name);
  PFElem.setPF(&PF);
  PFList.push_back(PFElem);
}

void PixelFormatList::insert(int index, char *format_name, PixelFormat PF) {
  PixelFormatListElement PFElem;
  PFElem.setName(format_name);
  PFElem.setPF(&PF);
  PFList.insert(getIterator(index), PFElem);
}

void PixelFormatList::remove(int index) {
  PFList.erase(getIterator(index));
}

list <PixelFormatListElement>::iterator PixelFormatList::getIterator(int index) {
  if ((index >= PFList.size()) || (index < 0))
    rdr::Exception("PixelFormatList:out of range");

  int i = 0;
  list <PixelFormatListElement>::iterator iter;
  for (iter = PFList.begin(); iter != PFList.end(); iter++) {
    if (i++ == index) break;
  }
  return iter;
}