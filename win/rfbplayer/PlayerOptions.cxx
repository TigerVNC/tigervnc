/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- PlayerOptions class

#include <rfbplayer/PlayerOptions.h>

using namespace rfb::win32;

PlayerOptions::PlayerOptions() { 
  writeDefaults();
};

void PlayerOptions::readFromRegistry() {
  try {
    PixelFormat *pPF = 0;
    int pfSize = sizeof(PixelFormat);
    RegKey regKey;
    regKey.createKey(HKEY_CURRENT_USER, _T("Software\\TightVnc\\RfbPlayer"));
    autoPlay = regKey.getBool(_T("AutoPlay"), DEFAULT_AUTOPLAY);
    autoDetectPF = regKey.getBool(_T("AutoDetectPixelFormat"), DEFAULT_AUTOPF);
    bigEndianFlag = regKey.getBool(_T("BigEndianFlag"), DEFAULT_BIG_ENDIAN);
    pixelFormatIndex = regKey.getInt(_T("PixelFormatIndex"), DEFAULT_PF_INDEX);
    regKey.getBinary(_T("PixelFormat"), (void**)&pPF, &pfSize, 
      &PixelFormat(32,24,0,1,255,255,255,16,8,0), sizeof(PixelFormat));
    setPF(pPF);
    acceptBell = regKey.getBool(_T("AcceptBell"), DEFAULT_ACCEPT_BELL);
    acceptCutText = regKey.getBool(_T("AcceptCutText"), DEFAULT_ACCEPT_CUT_TEXT);
    autoPlay = regKey.getBool(_T("AutoPlay"), DEFAULT_AUTOPLAY);
    askPixelFormat = regKey.getBool(_T("AskPixelFormat"), DEFAULT_ASK_PF);
    if (pPF) delete pPF;
  } catch (rdr::Exception e) {
    MessageBox(0, e.str(), "RFB Player", MB_OK | MB_ICONERROR);
  }
}

void PlayerOptions::writeToRegistry() {
  try {
    RegKey regKey;
    regKey.createKey(HKEY_CURRENT_USER, _T("Software\\TightVnc\\RfbPlayer"));
    regKey.setBool(_T("AutoPlay"), autoPlay);
    regKey.setBool(_T("AutoDetectPixelFormat"), autoDetectPF);
    regKey.setBool(_T("BigEndianFlag"), bigEndianFlag);
    regKey.setInt(_T("PixelFormatIndex"), pixelFormatIndex);
    regKey.setBinary(_T("PixelFormat"), &pixelFormat, sizeof(PixelFormat));
    regKey.setBool(_T("AcceptBell"), acceptBell);
    regKey.setBool(_T("AcceptCutText"), acceptCutText);
    regKey.setBool(_T("AutoPlay"), autoPlay);
    regKey.setBool(_T("AskPixelFormat"), askPixelFormat);
  } catch (rdr::Exception e) {
    MessageBox(0, e.str(), "RFB Player", MB_OK | MB_ICONERROR);
  }
}

void PlayerOptions::writeDefaults() {
  initTime = DEFAULT_INIT_TIME;
  playbackSpeed = DEFAULT_SPEED;
  autoDetectPF = DEFAULT_AUTOPF;
  bigEndianFlag = DEFAULT_BIG_ENDIAN;
  pixelFormatIndex = DEFAULT_PF_INDEX;
  pixelFormat = PixelFormat(32,24,0,1,255,255,255,16,8,0);
  frameScale = DEFAULT_FRAME_SCALE;
  autoPlay = DEFAULT_AUTOPLAY;
  fullScreen = DEFAULT_FULL_SCREEN;
  acceptBell = DEFAULT_ACCEPT_BELL; 
  acceptCutText = DEFAULT_ACCEPT_CUT_TEXT;
  loopPlayback = DEFAULT_LOOP_PLAYBACK;
  askPixelFormat = DEFAULT_ASK_PF; 
  commandLineParam = false;
}

void PlayerOptions::setPF(PixelFormat *newPF) {
  memcpy(&pixelFormat, newPF, sizeof(PixelFormat));
}

bool PlayerOptions::setPF(int rgb_order, int rm, int gm, int bm, bool big_endian) {
  PixelFormat newPF;
  
  // Calculate the colour bits per pixel
  int bpp = rm + gm + bm;
  if (bpp < 0) {
    return false;
  } else if (bpp <= 8 ) {
    newPF.bpp = 8;
  } else if (bpp <= 16) {
    newPF.bpp = 16;
  } else if (bpp <= 32) {
    newPF.bpp = 32;
  } else {
    return false;
  }
  newPF.depth = bpp;

  // Calculate the r, g and b bits shifts
  switch (rgb_order) {
  case RGB_ORDER:
    newPF.redShift = gm + bm;
    newPF.greenShift = bm;
    newPF.blueShift = 0;
    break;
  case RBG_ORDER:
    newPF.redShift = bm + gm;
    newPF.blueShift = gm;
    newPF.greenShift = 0;
    break;
  case GRB_ORDER:
    newPF.greenShift = rm + bm;
    newPF.redShift = bm;
    newPF.blueShift = 0;
    break;
  case GBR_ORDER:
    newPF.greenShift = bm + rm;
    newPF.blueShift = rm;
    newPF.redShift = 0;
    break;
  case BGR_ORDER:
    newPF.blueShift = gm + rm;
    newPF.greenShift = rm;
    newPF.redShift = 0;
    break;
  case BRG_ORDER:
    newPF.blueShift = rm + gm;
    newPF.redShift = gm;
    newPF.greenShift = 0;
    break;
  default:
    return false;
  }

  newPF.trueColour = true;
  newPF.bigEndian = big_endian;
  newPF.redMax = (1 << rm) - 1;
  newPF.greenMax = (1 << gm) - 1;
  newPF.blueMax = (1 << bm) - 1;
  setPF(&newPF);
  return true;
}