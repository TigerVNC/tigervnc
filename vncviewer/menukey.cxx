/* Copyright 2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <string.h>
#include <FL/Fl.H>

// FLTK can pull in the X11 headers on some systems
#ifndef XK_VoidSymbol
#define XK_MISCELLANY
#include <rfb/keysymdef.h>
#endif

#include "menukey.h"
#include "parameters.h"

static const MenuKeySymbol menuSymbols[] = {
  {"F1",          FL_F + 1,       0x3b, XK_F1},
  {"F2",          FL_F + 2,       0x3c, XK_F2},
  {"F3",          FL_F + 3,       0x3d, XK_F3},
  {"F4",          FL_F + 4,       0x3e, XK_F4},
  {"F5",          FL_F + 5,       0x3f, XK_F5},
  {"F6",          FL_F + 6,       0x40, XK_F6},
  {"F7",          FL_F + 7,       0x41, XK_F7},
  {"F8",          FL_F + 8,       0x42, XK_F8},
  {"F9",          FL_F + 9,       0x43, XK_F9},
  {"F10",         FL_F + 10,      0x44, XK_F10},
  {"F11",         FL_F + 11,      0x57, XK_F11},
  {"F12",         FL_F + 12,      0x58, XK_F12},
  {"Pause",       FL_Pause,       0xc6, XK_Pause},
  {"Scroll_Lock", FL_Scroll_Lock, 0x46, XK_Scroll_Lock},
  {"Escape",      FL_Escape,      0x01, XK_Escape},
  {"Insert",      FL_Insert,      0xd2, XK_Insert},
  {"Delete",      FL_Delete,      0xd3, XK_Delete},
  {"Home",        FL_Home,        0xc7, XK_Home},
  {"Page_Up",     FL_Page_Up,     0xc9, XK_Page_Up},
  {"Page_Down",   FL_Page_Down,   0xd1, XK_Page_Down},
};

int getMenuKeySymbolCount()
{
  return sizeof(menuSymbols)/sizeof(menuSymbols[0]);
}

const MenuKeySymbol* getMenuKeySymbols()
{
  return menuSymbols;
}

void getMenuKey(int *fltkcode, int *keycode, rdr::U32 *keysym)
{
  const char *menuKeyStr;

  menuKeyStr = menuKey;
  for(int i = 0; i < getMenuKeySymbolCount(); i++) {
    if (!strcmp(menuSymbols[i].name, menuKeyStr)) {
      *fltkcode = menuSymbols[i].fltkcode;
      *keycode = menuSymbols[i].keycode;
      *keysym = menuSymbols[i].keysym;
      return;
    }
  }

  *fltkcode = 0;
  *keycode = 0;
  *keysym = 0;
}
