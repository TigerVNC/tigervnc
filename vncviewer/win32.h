/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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

#ifndef __VNCVIEWER_WIN32_H__
#define __VNCVIEWER_WIN32_H__

#ifdef _MSC_VER
#define snprintf(str, n, format, ...) _snprintf_s(str, n, _TRUNCATE, format, __VA_ARGS__)
#endif

extern "C" {

int win32_enable_lowlevel_keyboard(HWND hwnd);
void win32_disable_lowlevel_keyboard(HWND hwnd);

int win32_vkey_to_keysym(UINT vkey, int extended);

int win32_has_altgr(void);
};

#endif
