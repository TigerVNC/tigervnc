/* Copyright (C) 2021 Vladimir Sukhonosov <xornet@xornet.org>
 * Copyright (C) 2021 Martins Mozeiko <martins.mozeiko@gmail.com>
 * All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/H264DecoderContext.h>

#ifdef H264_LIBAV
#include <rfb/H264LibavDecoderContext.h>
#define H264DecoderContextType H264LibavDecoderContext
#elif H264_WIN
#include <rfb/H264WinDecoderContext.h>
#define H264DecoderContextType H264WinDecoderContext
#endif

using namespace rfb;

H264DecoderContext *H264DecoderContext::createContext(const core::Rect &r)
{
  return new H264DecoderContextType(r);
}

H264DecoderContext::~H264DecoderContext()
{
}
