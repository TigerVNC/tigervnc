/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2012 Samuel Mannehed <samuel@cendio.se> for Cendio AB
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
#include "MonitorIndicesParameter.h"

#ifdef _WIN32
#include <vector>
#include <string>
#endif

#define SERVER_HISTORY_SIZE 20


extern rfb::IntParameter pointerEventInterval;
extern rfb::BoolParameter emulateMiddleButton;
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

extern rfb::BoolParameter maximize;
extern rfb::BoolParameter fullScreen;
extern rfb::StringParameter fullScreenMode;
extern rfb::BoolParameter fullScreenAllMonitors; // deprecated
extern MonitorIndicesParameter fullScreenSelectedMonitors;
extern rfb::StringParameter desktopSize;
extern rfb::StringParameter geometry;
extern rfb::BoolParameter remoteResize;

extern rfb::BoolParameter listenMode;

extern rfb::BoolParameter viewOnly;
extern rfb::BoolParameter shared;

extern rfb::BoolParameter acceptClipboard;
extern rfb::BoolParameter setPrimary;
extern rfb::BoolParameter sendClipboard;
#if !defined(WIN32) && !defined(__APPLE__)
extern rfb::BoolParameter sendPrimary;
extern rfb::StringParameter display;
#endif

extern rfb::StringParameter menuKey;

extern rfb::BoolParameter fullscreenSystemKeys;
extern rfb::BoolParameter alertOnFatalError;
extern rfb::BoolParameter reconnectOnError;

#ifndef WIN32
extern rfb::StringParameter via;
#endif

void saveViewerParameters(const char *filename, const char *servername=NULL);
char* loadViewerParameters(const char *filename);

#ifdef _WIN32
void loadHistoryFromRegKey(std::vector<std::string>& serverHistory);
void saveHistoryToRegKey(const std::vector<std::string>& serverHistory);
#endif

#endif
