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
    RegKey regKey;
    regKey.createKey(HKEY_CURRENT_USER, _T("Software\\TightVnc\\RfbPlayer"));
    autoPlay = regKey.getBool(_T("AutoPlay"), DEFAULT_AUTOPLAY);
    pixelFormat = regKey.getInt(_T("PixelFormat"), DEFAULT_PF);
    acceptBell = regKey.getBool(_T("AcceptBell"), DEFAULT_ACCEPT_BELL);
    acceptCutText = regKey.getBool(_T("AcceptCutText"), DEFAULT_ACCEPT_CUT_TEXT);
    autoStoreSettings = regKey.getBool(_T("AutoStoreSettings"), DEFAULT_STORE_SETTINGS);
    autoPlay = regKey.getBool(_T("AutoPlay"), DEFAULT_AUTOPLAY);
    loopPlayback = regKey.getBool(_T("LoopPlayback"), DEFAULT_LOOP_PLAYBACK);
    askPixelFormat = regKey.getBool(_T("AskPixelFormat"), DEFAULT_ASK_PF);
  } catch (rdr::Exception e) {
    MessageBox(0, e.str(), e.type(), MB_OK | MB_ICONERROR);
  }
}

void PlayerOptions::writeToRegistry() {
  try {
    RegKey regKey;
    regKey.createKey(HKEY_CURRENT_USER, _T("Software\\TightVnc\\RfbPlayer"));
    regKey.setBool(_T("AutoPlay"), autoPlay);
    regKey.setInt(_T("PixelFormat"), pixelFormat);
    regKey.setBool(_T("AcceptBell"), acceptBell);
    regKey.setBool(_T("AcceptCutText"), acceptCutText);
    regKey.setBool(_T("AutoStoreSettings"), autoStoreSettings);
    regKey.setBool(_T("AutoPlay"), autoPlay);
    regKey.setBool(_T("LoopPlayback"), loopPlayback);
    regKey.setBool(_T("AskPixelFormat"), askPixelFormat);
  } catch (rdr::Exception e) {
    MessageBox(0, e.str(), e.type(), MB_OK | MB_ICONERROR);
  }
}

void PlayerOptions::writeDefaults() {
  initTime = DEFAULT_INIT_TIME;
  playbackSpeed = DEFAULT_SPEED;
  pixelFormat = PF_AUTO;
  frameScale = DEFAULT_FRAME_SCALE;
  autoPlay = DEFAULT_AUTOPLAY;
  fullScreen = DEFAULT_FULL_SCREEN;
  acceptBell = DEFAULT_ACCEPT_BELL; 
  acceptCutText = DEFAULT_ACCEPT_CUT_TEXT;
  loopPlayback = DEFAULT_LOOP_PLAYBACK;
  askPixelFormat = DEFAULT_ASK_PF; 
  autoStoreSettings = DEFAULT_STORE_SETTINGS;
}