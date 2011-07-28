//
// "$Id: cgdebug.h 7913 2010-11-29 18:18:27Z greg.ercolano $"
//
// OS X Core Graphics debugging help for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// This file allows easier debugging of Mac OS X Core Graphics 
// code. This file is normally not included into any FLTK builds,
// but since it has proven to be tremendously useful in debugging
// the FLTK port to "Quartz", I decided to add this file in case
// more bugs show up.
//
// This header is activated by adding the following
// line to "config.h"
//   #include "src/cgdebug.h"
//
// Running "./configure" will remove this line from "config.h".
//
// When used erreanously, Core Graphics prints warnings to 
// stderr. This is helpful, however it is not possible to 
// associate a line number or source file with the warning message.
// This headr file outputs a trace of CG calls, interweaveing
// them with CG warnings.
//
// Matthias

#ifndef CGDEBUG
#define CGDEBUG

#include <stdio.h>
#include <Carbon/Carbon.h>

//+BitmapContextCreate
//+BitmapContextGetData
// ClipCGContextToRegion
// QDBeginCGContext
// QDEndCGContext

//+AddArc
//+AddLineToPoint
// ClipToRect
// ClosePath
//+ConcatCTM
//+DrawImage
// FillPath
// FillRect
// Flush
//+GetCTM
// MoveToPoint
//+Release
// RestoreGState
// SaveGState
//+ScaleCTM
//+SetLineCap
//+SetLineDash
//+SetLineJoin
//+SetLineWidth
//+SetRGBFillColor
//+SetRGBStrokeColor
//+SetShouldAntialias
//+SetTextMatrix
//+StrokePath
//+TranslateCTM

inline OSStatus dbgLocation(const char *file, int line) 
{
  fprintf(stderr, "%s:%d ", file, line);
  return 0;
}

inline OSStatus dbgEndl()     
{
  fprintf(stderr, "\n");
  return 0;
}


inline void dbgCGContextClipToRect(CGContextRef a, CGRect b)
{
  CGContextClipToRect(a, b);
}

#define CGContextClipToRect(a, b) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextClipToRect(a, b); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextFillRect(CGContextRef a, CGRect b)
{
  CGContextFillRect(a, b);
}

#define CGContextFillRect(a, b) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextFillRect(a, b); \
  fprintf(stderr, "\n"); }

inline OSStatus dbgQDEndCGContext(CGrafPtr a, CGContextRef *b) 
{
  return QDEndCGContext(a, b);
}

#define QDEndCGContext(a, b) ( \
  dbgLocation(__FILE__, __LINE__) + \
  dbgQDEndCGContext(a, b) + \
  dbgEndl() )

inline OSStatus dbgQDBeginCGContext(CGrafPtr a, CGContextRef *b) 
{
  return QDBeginCGContext(a, b);
}

#define QDBeginCGContext(a, b) ( \
  dbgLocation(__FILE__, __LINE__) + \
  dbgQDBeginCGContext(a, b) + \
  dbgEndl() )

inline void dbgClipCGContextToRegion(CGContextRef a, const Rect *b, RgnHandle c) 
{
  ClipCGContextToRegion(a, b, c);
}

#define ClipCGContextToRegion(a, b, c) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgClipCGContextToRegion(a, b, c); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextMoveToPoint(CGContextRef context, float x, float y)
{
  CGContextMoveToPoint(context, x, y);
}

#define CGContextMoveToPoint(a, b, c) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextMoveToPoint(a, b, c); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextFillPath(CGContextRef context)
{ 
  CGContextFillPath(context);
}

#define CGContextFillPath(a) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextFillPath(a); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextClosePath(CGContextRef context)
{ 
  CGContextClosePath(context);
}

#define CGContextClosePath(a) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextClosePath(a); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextFlush(CGContextRef context)
{ 
  CGContextFlush(context);
}

#define CGContextFlush(a) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextFlush(a); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextSaveGState(CGContextRef context)
{ 
  CGContextSaveGState(context);
}

#define CGContextSaveGState(a) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextSaveGState(a); \
  fprintf(stderr, "\n"); }

inline void dbgCGContextRestoreGState(CGContextRef context)
{ 
  CGContextRestoreGState(context);
}

#define CGContextRestoreGState(a) { \
  fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
  dbgCGContextRestoreGState(a); \
  fprintf(stderr, "\n"); }


#endif

//
// End of "$Id: cgdebug.h 7913 2010-11-29 18:18:27Z greg.ercolano $".
//

