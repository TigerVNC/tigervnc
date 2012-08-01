/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <rfb/Configuration.h>

extern rfb::IntParameter pointerEventInterval;
extern rfb::BoolParameter dotWhenNoCursor;

extern rfb::StringParameter passwordFile;

extern rfb::BoolParameter autoSelect;
extern rfb::BoolParameter fullColour;
extern rfb::AliasParameter fullColourAlias;
extern rfb::IntParameter lowColourLevel;
extern rfb::AliasParameter lowColourLevelAlias;
extern rfb::StringParameter preferredEncoding;
extern rfb::BoolParameter customCompressLevel;
extern rfb::IntParameter compressLevel;
extern rfb::BoolParameter noJpeg;
extern rfb::IntParameter qualityLevel;

#ifdef HAVE_FLTK_FULLSCREEN
extern rfb::BoolParameter fullScreen;
extern rfb::BoolParameter maximize;
#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
extern rfb::BoolParameter fullScreenAllMonitors;
#endif // HAVE_FLTK_FULLSCREEN_SCREENS
#endif // HAVE_FLTK_FULLSCREEN
extern rfb::StringParameter desktopSize;
extern rfb::BoolParameter remoteResize;

extern rfb::BoolParameter viewOnly;
extern rfb::BoolParameter shared;

extern rfb::BoolParameter acceptClipboard;
extern rfb::BoolParameter sendClipboard;
extern rfb::BoolParameter sendPrimary;

extern rfb::StringParameter menuKey;

extern rfb::BoolParameter fullscreenSystemKeys;

#endif
