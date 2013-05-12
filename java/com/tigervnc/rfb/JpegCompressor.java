/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2012 Brian P. Hinz
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

public class JpegCompressor {

  public static final int SUBSAMP_UNDEFINED = -1;
  public static final int SUBSAMP_NONE = 0;
  public static final int SUBSAMP_420 = 1;
  public static final int SUBSAMP_422 = 2;
  public static final int SUBSAMP_GRAY = 3;

  public static int subsamplingNum(String name) {
    if (name.equalsIgnoreCase("SUBSAMP_UNDEFINED")) return SUBSAMP_UNDEFINED;
    if (name.equalsIgnoreCase("SUBSAMP_NONE"))      return SUBSAMP_NONE;
    if (name.equalsIgnoreCase("SUBSAMP_420"))       return SUBSAMP_420;
    if (name.equalsIgnoreCase("SUBSAMP_422"))       return SUBSAMP_422;
    if (name.equalsIgnoreCase("SUBSAMP_GRAY"))      return SUBSAMP_GRAY;
    return SUBSAMP_UNDEFINED;
  }

  public static String subsamplingName(int num) {
    switch (num) {
    case SUBSAMP_UNDEFINED: return "SUBSAMP_UNDEFINED";
    case SUBSAMP_NONE:      return "SUBSAMP_NONE";
    case SUBSAMP_420:       return "SUBSAMP_420";
    case SUBSAMP_422:       return "SUBSAMP_422";
    case SUBSAMP_GRAY:      return "SUBSAMP_GRAY";
    default:                return "SUBSAMP_UNDEFINED";
    }
  }
}
