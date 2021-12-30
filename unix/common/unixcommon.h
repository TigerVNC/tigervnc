/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2015 Pierre Ossman for Cendio AB
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

#ifndef UNIXCOMMON_H
#define UNIXCOMMON_H

#include <map>

#include <rfb/ScreenSet.h>

typedef std::map<unsigned int, rdr::U32> OutputIdMap;

rfb::ScreenSet computeScreenLayout(OutputIdMap *outputIdMap);

unsigned int setScreenLayout(int fb_width, int fb_height, const rfb::ScreenSet& layout,
                             OutputIdMap *outputIdMap);

unsigned int tryScreenLayout(int fb_width, int fb_height, const rfb::ScreenSet& layout,
                             OutputIdMap *outputIdMap);

/*
 * FIXME: This is only exposed because we still have logic in XDesktop
 *        that we haven't integrated in setScreenLayout()
 */
int getPreferredScreenOutput(OutputIdMap *outputIdMap,
                             const std::set<unsigned int>& disabledOutputs);

#endif /* UNIXCOMMON_H */
