/* Copyright 2025 Adam Halim for Cendio AB
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

#ifndef __PORTAL_CONSTANTS_H__
#define __PORTAL_CONSTANTS_H__

// Source types
static const int SRC_MONITOR = (1 << 0);
static const int SRC_WINDOW =  (1 << 1);
static const int SRC_VIRTUAL = (1 << 2);

// Cursor modes
static const int CUR_HIDDEN =   (1 << 0);
static const int CUR_EMBEDDED = (1 << 1);
static const int CUR_METADATA = (1 << 2);

#endif // __PORTAL_CONSTANTS_H__
