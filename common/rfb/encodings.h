/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyeight (C) 2011 D. R. Commander.  All Rights Reserved.
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
#ifndef __RFB_ENCODINGS_H__
#define __RFB_ENCODINGS_H__

namespace rfb {

  const int encodingRaw = 0;
  const int encodingCopyRect = 1;
  const int encodingRRE = 2;
  const int encodingCoRRE = 4;
  const int encodingHextile = 5;
  const int encodingTight = 7;
  const int encodingZRLE = 16;
#ifdef HAVE_H264
  const int encodingH264 = 50;
#endif

  const int encodingMax = 255;

  const int pseudoEncodingXCursor = -240;
  const int pseudoEncodingCursor = -239;
  const int pseudoEncodingDesktopSize = -223;
  const int pseudoEncodingLEDState = -261;
  const int pseudoEncodingExtendedDesktopSize = -308;
  const int pseudoEncodingDesktopName = -307;
  const int pseudoEncodingFence = -312;
  const int pseudoEncodingContinuousUpdates = -313;
  const int pseudoEncodingCursorWithAlpha = -314;
  const int pseudoEncodingQEMUKeyEvent = -258;

  // TightVNC-specific
  const int pseudoEncodingLastRect = -224;
  const int pseudoEncodingQualityLevel0 = -32;
  const int pseudoEncodingQualityLevel9 = -23;
  const int pseudoEncodingCompressLevel0 = -256;
  const int pseudoEncodingCompressLevel9 = -247;

  // TurboVNC-specific
  const int pseudoEncodingFineQualityLevel0 = -512;
  const int pseudoEncodingFineQualityLevel100 = -412;
  const int pseudoEncodingSubsamp1X = -768;
  const int pseudoEncodingSubsamp4X = -767;
  const int pseudoEncodingSubsamp2X = -766;
  const int pseudoEncodingSubsampGray = -765;
  const int pseudoEncodingSubsamp8X = -764;
  const int pseudoEncodingSubsamp16X = -763;

  // VMware-specific
  const int pseudoEncodingVMwareCursor = 0x574d5664;
  const int pseudoEncodingVMwareCursorPosition = 0x574d5666;
  const int pseudoEncodingVMwareLEDState = 0x574d5668;

  // UltraVNC-specific
  const int pseudoEncodingExtendedClipboard = 0xC0A1E5CE;

  int encodingNum(const char* name);
  const char* encodingName(int num);
}
#endif
