#
# "$Id: CMakeLists.txt 8198 2011-01-06 10:24:58Z manolo $"
#
# Main CMakeLists.txt to build the FLTK project using CMake (www.cmake.org)
# Written by Andy Cedilnik and Julien Jomier
#
# Copyright 1998-2010 by Bill Spitzak and others.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA.
#
# Please report all bugs and problems on the following page:
#
#     http:#www.fltk.org/str.php
#

#######################################################################
# basic setup
#######################################################################
project(FLTK)
cmake_minimum_required(VERSION 2.6)

# The FLTK version
set(FLTK_VERSION_MAJOR "1")
set(FLTK_VERSION_MINOR "3")
set(FLTK_VERSION_PATCH "0")
set(FLTK_VERSION "${FLTK_VERSION_MAJOR}.${FLTK_VERSION_MINOR}")
set(FLTK_VERSION_FULL "${FLTK_VERSION}.${FLTK_VERSION_PATCH}")

# Search for modules in the FLTK source dir first
set(CMAKE_MODULE_PATH "${FLTK_SOURCE_DIR}/CMake")

include_directories(BEFORE ${FLTK_BINARY_DIR} ${FLTK_SOURCE_DIR})

#######################################################################
# platform dependent information
#######################################################################

include(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

if(APPLE)
   set(__APPLE_QUARTZ__ 1)
   set(HAVE_STRTOLL 1)
   set(HAVE_STRCASECMP 1)
   set(HAVE_DIRENT_H 1)
endif(APPLE)

if(WIN32)
   if(MSVC)
      add_definitions(-DWIN32_LEAN_AND_MEAN)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
   endif(MSVC)
endif(WIN32)

#######################################################################
# size of ints
include(CheckTypeSize)

CHECK_TYPE_SIZE(short SIZEOF_SHORT)
CHECK_TYPE_SIZE(int   SIZEOF_INT)
CHECK_TYPE_SIZE(long  SIZEOF_LONG)
CHECK_TYPE_SIZE("long long"  HAVE_LONG_LONG)

if(${SIZEOF_SHORT} MATCHES "^2$")
   set(U16 "unsigned short")
endif(${SIZEOF_SHORT} MATCHES "^2$")

if(${SIZEOF_INT} MATCHES "^4$")
   set(U32 "unsigned")
else()
   if(${SIZEOF_LONG} MATCHES "^4$")
      set(U32 "unsigned long")
   endif(${SIZEOF_LONG} MATCHES "^4$")
endif(${SIZEOF_INT} MATCHES "^4$")

if(${SIZEOF_INT} MATCHES "^8$")
   set(U64 "unsigned")
else()
   if(${SIZEOF_LONG} MATCHES "^8$")
      set(U64 "unsigned long")
   endif(${SIZEOF_LONG} MATCHES "^8$")
endif(${SIZEOF_INT} MATCHES "^8$")

#######################################################################
# check for headers, libraries and functions
#######################################################################
# headers
find_file(HAVE_DIRENT_H dirent.h)
find_file(HAVE_FREETYPE_H freetype.h PATH_SUFFIXES freetype2)
find_file(HAVE_LOCALE_H locale.h)
find_file(HAVE_NDIR_H ndir.h)
find_file(HAVE_PTHREAD_H pthread.h)
find_file(HAVE_SYS_DIR_H sys/dir.h)
find_file(HAVE_SYS_NDIR_H sys/ndir.h)
find_file(HAVE_SYS_SELECT_H sys/select.h)
find_file(HAVE_SYS_STDTYPES_H sys/stdtypes.h)

mark_as_advanced(HAVE_FREETYPE_H)
mark_as_advanced(HAVE_SYS_DIR_H)
mark_as_advanced(HAVE_SYS_NDIR_H HAVE_SYS_SELECT_H)
mark_as_advanced(HAVE_SYS_STDTYPES_H)

# where to find freetype headers
find_path(FREETYPE_PATH freetype/config/ftheader.h PATH_SUFFIXES freetype2)
if(FREETYPE_PATH)
   include_directories(${FREETYPE_PATH})
endif(FREETYPE_PATH)
mark_as_advanced(FREETYPE_PATH)

#######################################################################
# functions
include(CheckFunctionExists)

if(HAVE_DLFCN_H)
   set(CMAKE_REQUIRED_LIBRARIES dl)
endif(HAVE_DLFCN_H)
CHECK_FUNCTION_EXISTS(dlsym                  HAVE_DLSYM)

CHECK_FUNCTION_EXISTS(localeconv             HAVE_LOCALECONV)

CHECK_FUNCTION_EXISTS(scandir                HAVE_SCANDIR)
CHECK_FUNCTION_EXISTS(snprintf               HAVE_SNPRINTF)

CHECK_FUNCTION_EXISTS(strlcat                HAVE_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy                HAVE_STRLCPY)
CHECK_FUNCTION_EXISTS(strtoll                HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS(vsnprintf              HAVE_VSNPRINTF)

set(CMAKE_REQUIRED_LIBRARIES)

if(HAVE_SCANDIR AND NOT HAVE_SCANDIR_POSIX)
   set(MSG "POSIX compatible scandir")
   message(STATUS "Looking for ${MSG}")
   try_compile(V
      ${FLTK_BINARY_DIR}
      ${FLTK_SOURCE_DIR}/CMake/posixScandir.cxx
      )
   if(V)
      message(STATUS "${MSG} - found")
      set(HAVE_SCANDIR_POSIX 1 CACHE INTERNAL "")
   else()
      message(STATUS "${MSG} - not found")
      set(HAVE_SCANDIR_POSIX HAVE_SCANDIR_POSIX-NOTFOUND)
   endif(V)
endif(HAVE_SCANDIR AND NOT HAVE_SCANDIR_POSIX)
mark_as_advanced(HAVE_SCANDIR_POSIX)

#######################################################################
# options
#######################################################################
if(APPLE)
   option(OPTION_APPLE_X11 "use X11" OFF)
endif(APPLE)

if(NOT APPLE OR OPTION_APPLE_X11)
   include(FindX11)
   if(X11_FOUND)
      set(USE_X11 1)
   endif(X11_FOUND)
endif(NOT APPLE OR OPTION_APPLE_X11)

#######################################################################
option(OPTION_USE_POLL "use poll if available" OFF)

if(OPTION_USE_POLL)
   CHECK_FUNCTION_EXISTS(poll USE_POLL)
endif(OPTION_USE_POLL)

#######################################################################
option(OPTION_USE_THREADS "use multi-threading" ON)

if(OPTION_USE_THREADS)
   include(FindThreads)
endif(OPTION_USE_THREADS)

if(OPTION_USE_THREADS AND CMAKE_HAVE_THREADS_LIBRARY)
   add_definitions("-D_THREAD_SAFE -D_REENTRANT")
   set(USE_THREADS 1)
   set(FLTK_THREADS_FOUND TRUE)
else()
   set(FLTK_THREADS_FOUND FALSE)
endif(OPTION_USE_THREADS AND CMAKE_HAVE_THREADS_LIBRARY)

if(OPTION_USE_THREADS AND CMAKE_USE_PTHREADS_INIT)
   set(HAVE_PTHREAD 1)
   set(FLTK_PTHREADS_FOUND TRUE)
else()
   set(HAVE_PTHREAD 0)
   set(HAVE_PTHREAD_H 0)
   set(FLTK_PTHREADS_FOUND FALSE)
endif(OPTION_USE_THREADS AND CMAKE_USE_PTHREADS_INIT)

#######################################################################
if(X11_Xinerama_FOUND)
   option(OPTION_USE_XINERAMA "use lib Xinerama" ON)
endif(X11_Xinerama_FOUND)

if(OPTION_USE_XINERAMA)
   set(HAVE_XINERAMA ${X11_Xinerama_FOUND})
   include_directories(${X11_Xinerama_INCLUDE_PATH})
   set(FLTK_XINERAMA_FOUND TRUE)
else()
   set(FLTK_XINERAMA_FOUND FALSE)
endif(OPTION_USE_XINERAMA)

#######################################################################
if(X11_Xft_FOUND)
   option(OPTION_USE_XFT "use lib Xft" ON)
endif(X11_Xft_FOUND)

if(OPTION_USE_XFT)
   set(USE_XFT X11_Xft_FOUND)
   set(FLTK_XFT_FOUND TRUE)
else()
   set(FLTK_XFT_FOUND FALSE)
endif(OPTION_USE_XFT)

#######################################################################
if(X11_Xfixes_FOUND)
   option(OPTION_USE_XFIXES "use lib Xfixes" ON)
endif(X11_Xfixes_FOUND)

if(OPTION_USE_XFIXES)
   set(HAVE_XFIXES ${X11_Xfixes_FOUND})
endif(OPTION_USE_XFIXES)

#######################################################################
if(X11_Xcursor_FOUND)
   option(OPTION_USE_XCURSOR "use lib Xcursor" ON)
endif(X11_Xcursor_FOUND)

if(OPTION_USE_XCURSOR)
   set(HAVE_XCURSOR ${X11_Xcursor_FOUND})
endif(OPTION_USE_XCURSOR)

#######################################################################

add_subdirectory(src)

# generate config.h
configure_file(
   "${FLTK_SOURCE_DIR}/configh.cmake.in"
   "${FLTK_BINARY_DIR}/config.h"
   @ONLY
)
