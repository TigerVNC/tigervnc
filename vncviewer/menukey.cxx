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

#include <string.h>
#include <FL/Fl.H>

#include "menukey.h"
#include "parameters.h"

static const MenuKeySymbol menuSymbols[] = {
  {"F1", FL_F + 1},
  {"F2", FL_F + 2},
  {"F3", FL_F + 3},
  {"F4", FL_F + 4},
  {"F5", FL_F + 5},
  {"F6", FL_F + 6},
  {"F7", FL_F + 7},
  {"F8", FL_F + 8},
  {"F9", FL_F + 9},
  {"F10", FL_F + 10},
  {"F11", FL_F + 11},
  {"F12", FL_F + 12},
  {"Pause", FL_Pause},
  {"Print", FL_Print},
  {"Scroll_Lock", FL_Scroll_Lock},
  {"Escape", FL_Escape},
  {"Insert", FL_Insert},
  {"Delete", FL_Delete},
  {"Home", FL_Home},
  {"Page_Up", FL_Page_Up},
  {"Page_Down", FL_Page_Down},
};

int getMenuKeySymbolCount()
{
  return sizeof(menuSymbols)/sizeof(menuSymbols[0]);
}

const MenuKeySymbol* getMenuKeySymbols()
{
  return menuSymbols;
}

int getMenuKeyCode()
{
    const char *menuKeyStr;
    int menuKeyCode = 0;

    menuKeyStr = menuKey;
    for(int i = 0; i < getMenuKeySymbolCount(); i++)
      if (!strcmp(menuSymbols[i].name, menuKeyStr))
        menuKeyCode = menuSymbols[i].keycode;

    return menuKeyCode;
}
