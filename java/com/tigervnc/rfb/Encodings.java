/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

package com.tigervnc.rfb;

public class Encodings {

  public static final int encodingRaw = 0;
  public static final int encodingCopyRect = 1;
  public static final int encodingRRE = 2;
  public static final int encodingCoRRE = 4;
  public static final int encodingHextile = 5;
  public static final int encodingTight = 7;
  public static final int encodingZRLE = 16;

  public static final int encodingMax = 255;

  public static final int pseudoEncodingXCursor = -240;
  public static final int pseudoEncodingCursor = -239;
  public static final int pseudoEncodingDesktopSize = -223;
  public static final int pseudoEncodingExtendedDesktopSize = -308;
  public static final int pseudoEncodingDesktopName = -307;
  public static final int pseudoEncodingClientRedirect = -311;

  // TightVNC-specific
  public static final int pseudoEncodingLastRect = -224;
  public static final int pseudoEncodingQualityLevel0 = -32;
  public static final int pseudoEncodingQualityLevel9 = -23;
  public static final int pseudoEncodingCompressLevel0 = -256;
  public static final int pseudoEncodingCompressLevel9 = -247;

  public static int encodingNum(String name) {
    if (name.equalsIgnoreCase("raw"))      return encodingRaw;
    if (name.equalsIgnoreCase("copyRect")) return encodingCopyRect;
    if (name.equalsIgnoreCase("RRE"))      return encodingRRE;
    if (name.equalsIgnoreCase("coRRE"))    return encodingCoRRE;
    if (name.equalsIgnoreCase("hextile"))  return encodingHextile;
    if (name.equalsIgnoreCase("Tight"))    return encodingTight;
    if (name.equalsIgnoreCase("ZRLE"))     return encodingZRLE;
    return -1;
  }

  public static String encodingName(int num) {
    switch (num) {
    case encodingRaw:           return "raw";
    case encodingCopyRect:      return "copyRect";
    case encodingRRE:           return "RRE";
    case encodingCoRRE:         return "CoRRE";
    case encodingHextile:       return "hextile";
    case encodingTight:         return "Tight";
    case encodingZRLE:          return "ZRLE";
    default:                    return "[unknown encoding]";
    }
  }
}
