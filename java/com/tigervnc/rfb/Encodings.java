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
  public static final int pseudoEncodingFence = -312;
  public static final int pseudoEncodingContinuousUpdates = -313;
  public static final int pseudoEncodingCursorWithAlpha = -314;

  // TightVNC-specific
  public static final int pseudoEncodingLastRect = -224;
  public static final int pseudoEncodingQualityLevel0 = -32;
  public static final int pseudoEncodingQualityLevel9 = -23;
  public static final int pseudoEncodingCompressLevel0 = -256;
  public static final int pseudoEncodingCompressLevel9 = -247;

  // TurboVNC-specific
  public static final int pseudoEncodingFineQualityLevel0 = -512;
  public static final int pseudoEncodingFineQualityLevel100 = -412;
  public static final int pseudoEncodingSubsamp1X = -768;
  public static final int pseudoEncodingSubsamp4X = -767;
  public static final int pseudoEncodingSubsamp2X = -766;
  public static final int pseudoEncodingSubsampGray = -765;
  public static final int pseudoEncodingSubsamp8X = -764;
  public static final int pseudoEncodingSubsamp16X = -763;

  // VMware-specific
  public static final int pseudoEncodingVMwareCursor = 0x574d5664;

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
