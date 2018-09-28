/* Copyright 2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
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
#ifndef __KEYSYM_H__
#define __KEYSYM_H__

#include <rdr/types.h>

typedef struct {
  const char* name;
  int fltkcode;
  int keycode;
  rdr::U32 keysym;
} MenuKeySymbol;

void getMenuKey(int *fltkcode, int *keycode, rdr::U32 *keysym);
int getMenuKeySymbolCount();
const MenuKeySymbol* getMenuKeySymbols();

#endif
