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

// -=- PixelFormatList.h

// Definition of the PixelFormatList class, responsible for 
// controlling the list of supported pixel formats.

#include <list>

#include <rfb/Exception.h>
#include <rfb/PixelFormat.h>

#include <rfb_win32/registry.h>

// Definition indexes of the default pixel formats
#define PF_BGR233 0
#define PF_RGB555 1
#define PF_BGR555 2
#define PF_RGB565 3
#define PF_BGR565 4
#define PF_RGB888 5
#define PF_BGR888 6

using namespace rfb;
using namespace std;

// PixelFormatListElement class, it is 
// an item of the PixelFormatList list.

class PixelFormatListElement {
public:
  PixelFormatListElement() {
    format_name[0] = 0;
  }
  char format_name[256];
  PixelFormat PF;
  void setName(const char *name) {
    format_name[0] = '\0';
    strcpy(format_name, name);
    format_name[strlen(name)] = '\0';
  }
  void setPF(PixelFormat *_PF) {
    memcpy(&PF, _PF, sizeof(PixelFormat));
  }
};

class PixelFormatList {
public:
  PixelFormatList();

  PixelFormatListElement* operator[](int index);
  void add(char *format_name, PixelFormat PF);
  void insert(int index, char *format_name, PixelFormat PF);
  void remove(int index);

  void readUserDefinedPF(HKEY root, const char *keypath);
  void writeUserDefinedPF(HKEY root, const char *keypath);
  
  int count() { return PFList.size(); }
  int getDefaultPFCount() { return  PF_DEFAULT_COUNT; }
  int getUserPFCount() { return max(0, count() - PF_DEFAULT_COUNT); }
  int getIndexByPFName(const char *format_name);
  bool isDefaultPF(int index);

protected:
  list <PixelFormatListElement>::iterator getIterator(int index);
  list <PixelFormatListElement> PFList;
  int PF_DEFAULT_COUNT;
};