/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rfb/PixelFormat.h>
#include <rfb/Exception.h>
#include <rfb/ConnParams.h>
#include <rfb/SMsgWriter.h>
#include <rfb/ColourMap.h>
#include <rfb/TrueColourMap.h>
#include <rfb/PixelBuffer.h>
#include <rfb/ColourCube.h>
#include <rfb/TransImageGetter.h>

using namespace rfb;

TransImageGetter::TransImageGetter(bool econ)
  : PixelTransformer(econ), pb(0)
{
}

TransImageGetter::~TransImageGetter()
{
}

void TransImageGetter::init(PixelBuffer* pb_, const PixelFormat& out,
                            SMsgWriter* writer_, ColourCube* cube_)
{
  pb = pb_;
  writer = writer_;

  PixelTransformer::init(pb->getPF(), pb->getColourMap(), out, cube_,
                         cmCallback, this);
}

void TransImageGetter::setColourMapEntries(int firstCol, int nCols)
{
  PixelTransformer::setColourMapEntries(firstCol, nCols);
}

rdr::U8 *TransImageGetter::getRawPixelsRW(const Rect &r, int *stride)
{
  if (!offset.equals(Point(0, 0)))
    return pb->getPixelsRW(r.translate(offset.negate()), stride);
  else
    return pb->getPixelsRW(r, stride);
}

void TransImageGetter::getImage(void* outPtr, const Rect& r, int outStride)
{
  int inStride;
  const rdr::U8* inPtr = pb->getPixelsR(r.translate(offset.negate()), &inStride);

  if (!outStride) outStride = r.width();

  translateRect((void*)inPtr, inStride, Rect(0, 0, r.width(), r.height()),
                outPtr, outStride, Point(0, 0));
}

void TransImageGetter::cmCallback(int firstColour, int nColours,
                                  ColourMap* cm, void* data)
{
  TransImageGetter *self;

  assert(data);
  self = (TransImageGetter*)data;

  if (self->writer)
    self->writer->writeSetColourMapEntries(firstColour, nColours, cm);
}
