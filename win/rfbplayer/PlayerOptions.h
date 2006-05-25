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

// -=- PlayerOptions.h

// Definition of the PlayerOptions class, responsible for 
// storing & retrieving the RfbPlayer's options.

#include <rfb/PixelFormat.h>

#include <rfb_win32/registry.h>

#define PF_MODES 3

// The R, G and B values order in the pixel
#define RGB_ORDER 0
#define RBG_ORDER 1
#define GRB_ORDER 2
#define GBR_ORDER 3
#define BGR_ORDER 4
#define BRG_ORDER 5

// Default options values
//#define DEFAULT_PF 0
#define DEFAULT_PF_INDEX -1
#define DEFAULT_AUTOPF TRUE
#define DEFAULT_INIT_TIME -1
#define DEFAULT_SPEED 1.0
#define DEFAULT_FRAME_SCALE 100
#define DEFAULT_ACCEPT_BELL FALSE
#define DEFAULT_ACCEPT_CUT_TEXT FALSE
#define DEFAULT_BIG_ENDIAN FALSE
#define DEFAULT_LOOP_PLAYBACK FALSE
#define DEFAULT_ASK_PF FALSE
#define DEFAULT_AUTOPLAY FALSE
#define DEFAULT_FULL_SCREEN FALSE

using namespace rfb;

class PlayerOptions {
public:
  PlayerOptions();
  void readFromRegistry();
  void writeToRegistry();
  void writeDefaults();
  void setPF(PixelFormat *pf);
  bool setPF(int rgb_order, int rm, int gm, int bm, bool big_endian=false);
  long initTime;
  double playbackSpeed;
  bool autoPlay;
  bool fullScreen;
  bool autoDetectPF;
  bool bigEndianFlag;
  long pixelFormatIndex;
  PixelFormat pixelFormat;
  bool acceptBell;
  bool acceptCutText;
  bool loopPlayback;
  bool askPixelFormat;
  long frameScale;
  bool commandLineParam;
};