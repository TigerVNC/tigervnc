/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rfb/PixelFormat.h>
#include <rfb/Exception.h>
#include <rfb/ColourMap.h>
#include <rfb/TrueColourMap.h>
#include <rfb/PixelBuffer.h>
#include <rfb/ColourCube.h>
#include <rfb/PixelTransformer.h>

using namespace rfb;

static void noTransFn(void* table_,
                      const PixelFormat& inPF, void* inPtr, int inStride,
                      const PixelFormat& outPF, void* outPtr, int outStride,
                      int width, int height)
{
  rdr::U8* ip = (rdr::U8*)inPtr;
  rdr::U8* op = (rdr::U8*)outPtr;
  int inStrideBytes = inStride * (inPF.bpp/8);
  int outStrideBytes = outStride * (outPF.bpp/8);
  int widthBytes = width * (outPF.bpp/8);

  while (height > 0) {
    memcpy(op, ip, widthBytes);
    ip += inStrideBytes;
    op += outStrideBytes;
    height--;
  }
}

#define BPPOUT 8
#include "transInitTempl.h"
#define BPPIN 8
#include "transTempl.h"
#undef BPPIN
#define BPPIN 16
#include "transTempl.h"
#undef BPPIN
#define BPPIN 32
#include "transTempl.h"
#undef BPPIN
#undef BPPOUT

#define BPPOUT 16
#include "transInitTempl.h"
#define BPPIN 8
#include "transTempl.h"
#undef BPPIN
#define BPPIN 16
#include "transTempl.h"
#undef BPPIN
#define BPPIN 32
#include "transTempl.h"
#undef BPPIN
#undef BPPOUT

#define BPPOUT 32
#include "transInitTempl.h"
#define BPPIN 8
#include "transTempl.h"
#undef BPPIN
#define BPPIN 16
#include "transTempl.h"
#undef BPPIN
#define BPPIN 32
#include "transTempl.h"
#undef BPPIN
#undef BPPOUT


// Translation functions.  Note that transSimple* is only used for 8/16bpp and
// transRGB* is used for 16/32bpp

static transFnType transSimpleFns[][3] = {
  { transSimple8to8,  transSimple8to16,  transSimple8to32 },
  { transSimple16to8, transSimple16to16, transSimple16to32 },
};
static transFnType transRGBFns[][3] = {
  { transRGB16to8, transRGB16to16, transRGB16to32 },
  { transRGB32to8, transRGB32to16, transRGB32to32 }
};
static transFnType transRGBCubeFns[][3] = {
  { transRGBCube16to8, transRGBCube16to16, transRGBCube16to32 },
  { transRGBCube32to8, transRGBCube32to16, transRGBCube32to32 }
};

// Table initialisation functions.

typedef void (*initCMtoTCFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                 ColourMap* cm, const PixelFormat& outPF);
typedef void (*initTCtoTCFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                 const PixelFormat& outPF);
typedef void (*initCMtoCubeFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                   ColourMap* cm, ColourCube* cube);
typedef void (*initTCtoCubeFnType)(rdr::U8** tablep, const PixelFormat& inPF,
                                   ColourCube* cube);


static initCMtoTCFnType initSimpleCMtoTCFns[] = {
    initSimpleCMtoTC8, initSimpleCMtoTC16, initSimpleCMtoTC32
};

static initTCtoTCFnType initSimpleTCtoTCFns[] = {
    initSimpleTCtoTC8, initSimpleTCtoTC16, initSimpleTCtoTC32
};

static initCMtoCubeFnType initSimpleCMtoCubeFns[] = {
    initSimpleCMtoCube8, initSimpleCMtoCube16, initSimpleCMtoCube32
};

static initTCtoCubeFnType initSimpleTCtoCubeFns[] = {
    initSimpleTCtoCube8, initSimpleTCtoCube16, initSimpleTCtoCube32
};

static initTCtoTCFnType initRGBTCtoTCFns[] = {
    initRGBTCtoTC8, initRGBTCtoTC16, initRGBTCtoTC32
};

static initTCtoCubeFnType initRGBTCtoCubeFns[] = {
    initRGBTCtoCube8, initRGBTCtoCube16, initRGBTCtoCube32
};


PixelTransformer::PixelTransformer(bool econ)
  : economic(econ), cmCallback(0), cube(0), table(0), transFn(0)
{
}

PixelTransformer::~PixelTransformer()
{
  delete [] table;
}

void PixelTransformer::init(const PixelFormat& inPF_, ColourMap* inCM_,
                            const PixelFormat& outPF_, ColourCube* cube_,
                            setCMFnType cmCallback_, void *cbData_)
{
  inPF = inPF_;
  inCM = inCM_;

  outPF = outPF_;
  cube = cube_;
  cmCallback = cmCallback_;
  cbData = cbData_;

  if (table)
    delete [] table;
  table = NULL;
  transFn = NULL;

  if ((inPF.bpp != 8) && (inPF.bpp != 16) && (inPF.bpp != 32))
    throw Exception("PixelTransformer: bpp in not 8, 16 or 32");

  if ((outPF.bpp != 8) && (outPF.bpp != 16) && (outPF.bpp != 32))
    throw Exception("PixelTransformer: bpp out not 8, 16 or 32");

  if (!outPF.trueColour) {
    if (outPF.bpp != 8)
      throw Exception("PixelTransformer: outPF has color map but not 8bpp");

    if (!inPF.trueColour) {
      if (inPF.bpp != 8)
        throw Exception("PixelTransformer: inPF has colorMap but not 8bpp");
      if (!inCM)
        throw Exception("PixelTransformer: inPF has colorMap but no colour map specified");

      // CM to CM/Cube

      if (cube) {
        transFn = transSimpleFns[0][0];
        (*initSimpleCMtoCubeFns[0]) (&table, inPF, inCM, cube);
      } else {
        transFn = noTransFn;
        setColourMapEntries(0, 256);
      }
      return;
    }

    // TC to CM/Cube

    ColourCube defaultCube(6,6,6);
    if (!cube) cube = &defaultCube;

    if ((inPF.bpp > 16) || (economic && (inPF.bpp == 16))) {
      transFn = transRGBCubeFns[inPF.bpp/32][0];
      (*initRGBTCtoCubeFns[0]) (&table, inPF, cube);
    } else {
      transFn = transSimpleFns[inPF.bpp/16][0];
      (*initSimpleTCtoCubeFns[0]) (&table, inPF, cube);
    }

    if (cube != &defaultCube)
      return;

    if (!cmCallback)
      throw Exception("PixelTransformer: Neither colour map callback nor colour cube provided");

    cmCallback(0, 216, cube, cbData);
    cube = 0;
    return;
  }

  if (inPF.equal(outPF)) {
    transFn = noTransFn;
    return;
  }

  if (!inPF.trueColour) {

    // CM to TC

    if (inPF.bpp != 8)
      throw Exception("PixelTransformer: inPF has colorMap but not 8bpp");
    if (!inCM)
      throw Exception("PixelTransformer: inPF has colorMap but no colour map specified");

    transFn = transSimpleFns[0][outPF.bpp/16];
    (*initSimpleCMtoTCFns[outPF.bpp/16]) (&table, inPF, inCM, outPF);
    return;
  }

  // TC to TC

  if ((inPF.bpp > 16) || (economic && (inPF.bpp == 16))) {
    transFn = transRGBFns[inPF.bpp/32][outPF.bpp/16];
    (*initRGBTCtoTCFns[outPF.bpp/16]) (&table, inPF, outPF);
  } else {
    transFn = transSimpleFns[inPF.bpp/16][outPF.bpp/16];
    (*initSimpleTCtoTCFns[outPF.bpp/16]) (&table, inPF, outPF);
  }
}

const PixelFormat &PixelTransformer::getInPF() const
{
  return inPF;
}

const ColourMap *PixelTransformer::getInColourMap() const
{
  return inCM;
}

const PixelFormat &PixelTransformer::getOutPF() const
{
  return outPF;
}

const ColourCube *PixelTransformer::getOutColourCube() const
{
  return cube;
}

void PixelTransformer::setColourMapEntries(int firstCol, int nCols)
{
  if (nCols == 0)
    nCols = (1 << inPF.depth) - firstCol;

  if (inPF.trueColour) return; // shouldn't be called in this case

  if (outPF.trueColour) {
    (*initSimpleCMtoTCFns[outPF.bpp/16]) (&table, inPF, inCM, outPF);
  } else if (cube) {
    (*initSimpleCMtoCubeFns[outPF.bpp/16]) (&table, inPF, inCM, cube);
  } else {
    if (!cmCallback)
      throw Exception("PixelTransformer: Neither colour map callback nor colour cube provided");
    cmCallback(firstCol, nCols, inCM, cbData);
  }
}

void PixelTransformer::translatePixels(void* inPtr, void* outPtr,
                                       int nPixels) const
{
  if (!transFn)
    throw Exception("PixelTransformer: not initialised yet");

  (*transFn)(table, inPF, inPtr, nPixels,
             outPF, outPtr, nPixels, nPixels, 1);
}

void PixelTransformer::translateRect(void* inPtr, int inStride,
                                     Rect inRect,
                                     void* outPtr, int outStride,
                                     Point outCoord) const
{
  char *in, *out;

  if (!transFn)
    throw Exception("PixelTransformer: not initialised yet");

  in = (char*)inPtr;
  in += inPF.bpp/8 * inRect.tl.x;
  in += (inStride * inPF.bpp/8) * inRect.tl.y;

  out = (char*)outPtr;
  out += outPF.bpp/8 * outCoord.x;
  out += (outStride * outPF.bpp/8) * outCoord.y;

  (*transFn)(table, inPF, in, inStride,
             outPF, out, outStride,
             inRect.width(), inRect.height());
}

bool PixelTransformer::willTransform(void)
{
  return transFn != NULL && transFn != noTransFn;
}
