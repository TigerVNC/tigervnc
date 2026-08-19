/* Copyright 2019 Pierre Ossman for Cendio AB
 * Copyright (C) 2026 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

public class ClipboardTypes {

  // Formats
  public static final int clipboardUTF8  = 1 << 0;
  public static final int clipboardRTF   = 1 << 1;
  public static final int clipboardHTML  = 1 << 2;
  public static final int clipboardDIB   = 1 << 3;
  public static final int clipboardFiles = 1 << 4;

  public static final int clipboardFormatMask = 0x0000ffff;

  // Actions
  public static final int clipboardCaps    = 1 << 24;
  public static final int clipboardRequest = 1 << 25;
  public static final int clipboardPeek    = 1 << 26;
  public static final int clipboardNotify  = 1 << 27;
  public static final int clipboardProvide = 1 << 28;

  public static final int clipboardActionMask = 0xff000000;
}
