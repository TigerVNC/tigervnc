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

#include <core/Configuration.h>

#include "MonitorIndicesParameter.h"

#ifdef _WIN32
#include <list>
#include <string>
#endif

#define SERVER_HISTORY_SIZE 20


extern core::IntParameter pointerEventInterval;
extern core::BoolParameter emulateMiddleButton;
extern core::BoolParameter dotWhenNoCursor; // deprecated
extern core::BoolParameter alwaysCursor;
extern core::EnumParameter cursorType;

extern core::StringParameter passwordFile;

extern core::BoolParameter autoSelect;
extern core::BoolParameter fullColour;
extern core::AliasParameter fullColourAlias;
extern core::IntParameter lowColourLevel;
extern core::AliasParameter lowColourLevelAlias;
extern core::EnumParameter preferredEncoding;
extern core::BoolParameter customCompressLevel;
extern core::IntParameter compressLevel;
extern core::BoolParameter noJpeg;
extern core::IntParameter qualityLevel;

extern core::BoolParameter maximize;
extern core::BoolParameter fullScreen;
extern core::EnumParameter fullScreenMode;
extern core::BoolParameter fullScreenAllMonitors; // deprecated
extern MonitorIndicesParameter fullScreenSelectedMonitors;
extern core::StringParameter desktopSize;
extern core::StringParameter geometry;
extern core::BoolParameter remoteResize;

extern core::BoolParameter listenMode;

extern core::BoolParameter viewOnly;
extern core::BoolParameter shared;

extern core::BoolParameter acceptClipboard;
extern core::BoolParameter setPrimary;
extern core::BoolParameter sendClipboard;
#if !defined(WIN32) && !defined(__APPLE__)
extern core::BoolParameter sendPrimary;
extern core::StringParameter display;
#endif

extern core::EnumListParameter shortcutModifiers;

extern core::BoolParameter fullscreenSystemKeys;
extern core::BoolParameter alertOnFatalError;
extern core::BoolParameter reconnectOnError;

#ifndef WIN32
extern core::StringParameter via;
#endif

void saveViewerParameters(const char *filename, const char *servername=nullptr);
char* loadViewerParameters(const char *filename);

#ifdef _WIN32
std::list<std::string> loadHistoryFromRegKey();
void saveHistoryToRegKey(const std::list<std::string>& serverHistory);
#endif

#endif
