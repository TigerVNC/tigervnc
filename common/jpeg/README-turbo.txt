*******************************************************************************
**     Background
*******************************************************************************

libjpeg-turbo is a high-speed version of libjpeg for x86 and x86-64 processors
which uses SIMD instructions (MMX, SSE2, etc.) to accelerate baseline JPEG
compression and decompression.  libjpeg-turbo is generally 2-4x as fast
as the unmodified version of libjpeg, all else being equal.

libjpeg-turbo was originally based on libjpeg/SIMD by Miyasaka Masaru, but
the TigerVNC and VirtualGL projects made numerous enhancements to the codec,
including improved support for Mac OS X, 64-bit support, support for 32-bit
and big endian pixel formats, accelerated Huffman encoding/decoding, and
various bug fixes.  The goal was to produce a fully open source codec that
could replace the partially closed source TurboJPEG/IPP codec used by VirtualGL
and TurboVNC.  libjpeg-turbo generally performs in the range of 80-120% of
TurboJPEG/IPP.  It is faster in some areas but slower in others.

It was decided to split libjpeg-turbo into a separate SDK so that other
projects could take advantage of this technology.  The libjpeg-turbo shared
libraries can be used as drop-in replacements for libjpeg on most systems.


*******************************************************************************
**     License
*******************************************************************************

Some of the optimizations to the Huffman encoder (jchuff.c) and decoder
(jdhuff.c) were borrowed from VirtualGL, and thus any distribution of
libjpeg-turbo which includes those optimizations must, as a whole, be subject
to the terms of the wxWindows Library Licence, Version 3.1.  A copy of this
license can be found in this directory under LICENSE.txt.  The wxWindows
Library License is based on the LGPL but includes provisions which allow the
Library to be statically linked into proprietary libraries and applications
without requiring the resulting binaries to be distributed under the terms of
the LGPL.

The rest of the source code, apart from the Huffman codec optimizations, falls
under a less restrictive, BSD-style license (see README.)  You can choose to
distribute libjpeg-turbo, as a whole, under this BSD-style license by simply
replacing the optimized jchuff.c and jdhuff.c with their unoptimized
counterparts from the libjpeg v6b source.


*******************************************************************************
**     Using libjpeg-turbo
*******************************************************************************

=============================
Replacing libjpeg at Run Time
=============================

If a Unix application is dynamically linked with libjpeg, then you can replace
libjpeg with libjpeg-turbo at run time by manipulating the LD_LIBRARY_PATH.
For instance:

  [Using libjpeg]
  > time cjpeg <vgl_5674_0098.ppm >vgl_5674_0098.jpg
  real  0m0.392s
  user  0m0.074s
  sys   0m0.020s

  [Using libjpeg-turbo]
  > export LD_LIBRARY_PATH=/opt/libjpeg-turbo/{lib}:$LD_LIBRARY_PATH
  > time cjpeg <vgl_5674_0098.ppm >vgl_5674_0098.jpg
  real  0m0.109s
  user  0m0.029s
  sys   0m0.010s

NOTE: {lib} can be lib, lib32, lib64, or lib/64, depending on the O/S and
architecture.

System administrators can also replace the libjpeg sym links in /usr/{lib} with
links to the libjpeg dynamic library located in /opt/libjpeg-turbo/{lib}.  This
will effectively accelerate every dynamically linked libjpeg application on the
system.

The Windows distribution of the libjpeg-turbo SDK installs jpeg62.dll into
c:\libjpeg-turbo\bin, and the PATH environment variable can be modified such
that this directory is searched before any others that might contain
jpeg62.dll.  However, if jpeg62.dll also exists in an application's install
directory, then Windows will load the application's version of it first.  Thus,
if an application ships with jpeg62.dll, then back up the application's version
of jpeg62.dll and copy c:\libjpeg-turbo\bin\jpeg62.dll into the application's
install directory to accelerate it.

The version of jpeg62.dll distributed in the libjpeg-turbo SDK requires the
Visual C++ 2008 C run time DLL (msvcr90.dll).  This library ships with more
recent versions of Windows, but users of older versions can obtain it from the
Visual C++ 2008 Redistributable Package, which is available as a free download
from Microsoft's web site.

NOTE:  Features of libjpeg which require passing a C run time structure, such
as a file handle, from an application to libjpeg will probably not work with
the distributed version of jpeg62.dll unless the application is also built to
use the Visual C++ 2008 C run time DLL.  In particular, this affects
jpeg_stdio_dest() and jpeg_stdio_src().

Mac applications typically embed their own copies of libjpeg.62.dylib inside
the (hidden) application bundle, so it is not possible to globally replace
libjpeg on OS X systems.  If an application uses a shared library version of
libjpeg, then it may be possible to replace the application's version of it.
This would generally involve copying libjpeg.62.dylib into the appropriate
place in the application bundle and using install_name_tool to repoint the
dylib to the new directory.  This requires an advanced knowledge of OS X and
would not survive an upgrade or a re-install of the application.  Thus, it is
not recommended for most users.

=======================
Replacing TurboJPEG/IPP
=======================

libjpeg-turbo is a drop-in replacement for the TurboJPEG/IPP SDK used by
VirtualGL 2.1.x and TurboVNC 0.6 (and prior.)  libjpeg-turbo contains a wrapper
library (TurboJPEG/OSS) that emulates the TurboJPEG API using libjpeg-turbo
instead of the closed source Intel Performance Primitives.  You can replace the
TurboJPEG/IPP package on Linux systems with the libjpeg-turbo package in order
to make existing releases of VirtualGL 2.1.x and TurboVNC use the new codec at
run time.  Note that the 64-bit libjpeg-turbo packages contain only 64-bit
binaries, whereas the TurboJPEG/IPP 64-bit packages contained both 64-bit and
32-bit binaries.  Thus, to replace a TurboJPEG/IPP 64-bit package, install
both the 64-bit and 32-bit versions of libjpeg-turbo.

You can also build the VirtualGL 2.1.x and TurboVNC 0.6 source code with
the libjpeg-turbo SDK instead of TurboJPEG/IPP.  It should work identically.
libjpeg-turbo also includes static library versions of TurboJPEG/OSS, which
are used to build TurboVNC 1.0 and later.

========================================
Using libjpeg-turbo in Your Own Programs
========================================

For the most part, libjpeg-turbo should work identically to libjpeg, so in
most cases, an application can be built against libjpeg and then run against
libjpeg-turbo.  On Unix systems, you can build against libjpeg-turbo instead
of libjpeg by setting

  CPATH=/opt/libjpeg-turbo/include
  and
  LIBRARY_PATH=/opt/libjpeg-turbo/{lib}

({lib} = lib32 or lib64, depending on whether you are building a 32-bit or a
64-bit application.)

If using Cygwin, then set

  CPATH=/cygdrive/c/libjpeg-turbo-gcc[64]/include
  and
  LIBRARY_PATH=/cygdrive/c/libjpeg-turbo-gcc[64]/lib

If using MinGW, then set

  CPATH=/c/libjpeg-turbo-gcc[64]/include
  and
  LIBRARY_PATH=/c/libjpeg-turbo-gcc[64]/lib

Building against libjpeg-turbo is useful, for instance, if you want to build an
application that leverages the libjpeg-turbo colorspace extensions (see below.)
On Linux and Solaris systems, you would still need to manipulate the
LD_LIBRARY_PATH or sym links appropriately to use libjpeg-turbo at run time.
On such systems, you can pass -R /opt/libjpeg-turbo/{lib} to the linker to
force the use of libjpeg-turbo at run time rather than libjpeg (also useful if
you want to leverage the colorspace extensions), or you can link against the
libjpeg-turbo static library.

To force a Linux, Solaris, or MinGW application to link against the static
version of libjpeg-turbo, you can use the following linker options:

  -Wl,-Bstatic -ljpeg -Wl,-Bdynamic

On OS X, simply add /opt/libjpeg-turbo/lib/libjpeg.a to the linker command
line (this also works on Linux and Solaris.)

To build Visual C++ applications using libjpeg-turbo, add
c:\libjpeg-turbo[64]\include to your system or user INCLUDE environment
variable and c:\libjpeg-turbo[64]\lib to your system or user LIB environment
variable, and then link against either jpeg.lib (to use jpeg62.dll) or
jpeg-static.lib (to use the static version of libjpeg-turbo.)

=====================
Colorspace Extensions
=====================

libjpeg-turbo includes extensions which allow JPEG images to be compressed
directly from (and decompressed directly to) buffers which use BGR, BGRA,
RGBA, ABGR, and ARGB pixel ordering.  This is implemented with six new
colorspace constants:

  JCS_EXT_RGB   /* red/green/blue */
  JCS_EXT_RGBX  /* red/green/blue/x */
  JCS_EXT_BGR   /* blue/green/red */
  JCS_EXT_BGRX  /* blue/green/red/x */
  JCS_EXT_XBGR  /* x/blue/green/red */
  JCS_EXT_XRGB  /* x/red/green/blue */

Setting cinfo.in_color_space (compression) or cinfo.out_color_space
(decompression) to one of these values will cause libjpeg-turbo to read the
red, green, and blue values from (or write them to) the appropriate position in
the pixel when YUV conversion is performed.

Your application can check for the existence of these extensions at compile
time with:

  #ifdef JCS_EXTENSIONS

At run time, attempting to use these extensions with a version of libjpeg
that doesn't support them will result in a "Bogus input colorspace" error.
