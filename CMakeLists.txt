#
# Setup
#

cmake_minimum_required(VERSION 2.8)

# Internal cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

include(CMakeMacroLibtoolFile)

project(tigervnc)
set(VERSION 1.0.90)

# The RC version must always be four comma-separated numbers
set(RCVERSION 1,0,90,0)

# Manual toggle until we can deprecate the old viewers
option(BUILD_NEW_VNCVIEWER "Build the new FLTK based vncviewer instead of the old ones" ON)

# Compatibility variables for the migration from autotools
add_definitions(-DPACKAGE_NAME="${CMAKE_PROJECT_NAME}")
add_definitions(-DPACKAGE_VERSION="${VERSION}")
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")

# Try to encode today's date into the build id. We assume that MSVC
# means we need to use a native Windows method, otherwise we assume
# some kind of Unix system. The id will be empty if things fail.
set(BUILD "")
if(MSVC)
  execute_process(COMMAND "${CMAKE_SOURCE_DIR}/cmake/getdate.bat"
    OUTPUT_VARIABLE BUILD)
else()
  execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
endif()

if(NOT BUILD)
  set(BUILD "")
else()
  string(REGEX REPLACE "\n" "" BUILD ${BUILD})
endif()

# Default to optimised builds instead of debug ones. Our code has no bugs ;)
# (CMake makes it fairly easy to toggle this back to Debug if needed)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# This only works if building from the command line.  There is currently no way
# to set a variable's value based on the build type when using the MSVC IDE.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD "${BUILD}d")
endif()

message(STATUS "VERSION = ${VERSION}, BUILD = ${BUILD}")

if(NOT DEFINED BUILD_WINVNC)
  if(MSVC)
    set(BUILD_WINVNC 1)
  else()
    set(BUILD_WINVNC 0)
  endif()
endif()

if(MSVC)
  # Use the static C library for all build types
  foreach(var CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
    endif()
  endforeach()

  # NOTE: 4244 and 4267 are 64-bit to 32-bit conversion warnings, so normally
  # it is not a good idea to disable them, but we do this to duplicate the
  # behavior of GCC, which is less strict.
  add_definitions(-wd4244 -wd4267 -wd4800 -wd4996)
endif()

# Minimum version is Windows 2000 (5.0)
if(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P MATCHES 8)
    add_definitions(-D_WIN32_IE=0x0500 -D_WIN32_WINNT=0x0500)
  else()
    # Win64 doesn't like us requesting a Windows version that didn't have
    # 64-bit support. Request XP (5.1) instead.
    add_definitions(-D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501)
  endif()
endif()

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
  message(STATUS "64-bit build")
else()
  message(STATUS "32-bit build")
endif()

# CMake doesn't properly support resource compilation with MinGW.  Boo!
if(MINGW)
  if(NOT DEFINED RC)
    set(CMAKE_RC_COMPILER_INIT windres)
  else()
    set(CMAKE_RC_COMPILER_INIT ${RC})
  endif()
  enable_language(RC)
  message(STATUS "Resource compiler: ${CMAKE_RC_COMPILER}")
  set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> <DEFINES> -o <OBJECT> --output-format=coff <SOURCE>")
endif()

# X11 stuff. It's in a if() so that we can say REQUIRED
if(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
endif()

# Check for zlib
find_package(ZLIB)
option(USE_INCLUDED_ZLIB "Force use of the bundled zlib")
if(NOT ZLIB_FOUND)
  message(STATUS "System zlib not found. Using included zlib")
  set(USE_INCLUDED_ZLIB 1)
endif()

# Check for gettext
option(ENABLE_NLS "Enable translation of program messages" ON)
if(ENABLE_NLS)
  # Tools
  find_package(Gettext)
  set(LOCALE_DIR "${CMAKE_INSTALL_PREFIX}/share/locale")

  # Gettext needs iconv
  find_package(Iconv)

  if(ICONV_FOUND)
    # Headers and libraries (copied from licq)
    set(GETTEXT_FOUND FALSE)

    find_path(GETTEXT_INCLUDE_DIR libintl.h)
    if(GETTEXT_INCLUDE_DIR)
      set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
      check_function_exists(dgettext LIBC_HAS_DGETTEXT)
      if(LIBC_HAS_DGETTEXT)
        set(GETTEXT_FOUND TRUE)
      else()
        find_library(LIBINTL_LIBRARY NAMES intl libintl)
        check_library_exists(${LIBINTL_LIBRARY} "dgettext" "" LIBINTL_HAS_DGETTEXT)
        if(LIBINTL_HAS_DGETTEXT)
          set(GETTEXT_LIBRARIES ${LIBINTL_LIBRARY} ${ICONV_LIBRARIES})
          set(GETTEXT_FOUND TRUE)
        endif()
      endif()
      set(CMAKE_REQUIRED_LIBRARIES)
    endif()
  endif()

  if(NOT GETTEXT_FOUND)
    message(WARNING "Gettext NOT found.  Native Language Support disabled.")
    set(ENABLE_NLS 0)
  endif()
endif()

# Check for libjpeg
find_package(JPEG REQUIRED)

# Warn if it doesn't seem to be the accelerated libjpeg that's found
set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARIES})
set(CMAKE_REQUIRED_FLAGS -I${JPEG_INCLUDE_DIR})

check_c_source_compiles("#include <stdio.h>\n#include <jpeglib.h>\nint main(int c, char** v) { return JCS_EXT_RGBX; }" FOUND_JPEG_TURBO)

set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_FLAGS)

if(NOT FOUND_JPEG_TURBO)
  message(STATUS "WARNING: You are not using libjpeg-turbo. Performance will suffer.")
endif()

# Check for FLTK
if(BUILD_NEW_VNCVIEWER)
  set(FLTK_SKIP_FLUID TRUE)
  set(FLTK_SKIP_OPENGL TRUE)
  set(FLTK_SKIP_IMAGES TRUE)
  set(FLTK_SKIP_FORMS TRUE)
  find_package(FLTK COMPONENTS REQUIRED)

  if(NOT APPLE)
    # No proper handling for extra X11 libs that FLTK might need...
    if(X11_Xft_FOUND)
      set(FLTK_LIBRARIES ${FLTK_LIBRARIES} ${X11_Xft_LIB})
    endif()
    if(X11_Xinerama_FOUND)
      set(FLTK_LIBRARIES ${FLTK_LIBRARIES} ${X11_Xinerama_LIB})
    endif()
    if(X11_Xfixes_FOUND)
      set(FLTK_LIBRARIES ${FLTK_LIBRARIES} ${X11_Xfixes_LIB})
    endif()
    if(X11_Xcursor_FOUND)
      set(FLTK_LIBRARIES ${FLTK_LIBRARIES} ${X11_Xcursor_LIB})
    endif()
  endif()

  set(CMAKE_REQUIRED_INCLUDES ${FLTK_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${FLTK_LIBRARIES})

  # FLTK STR #2599
  check_cxx_source_compiles("#include <FL/Fl_Widget.H>\nint main(int c, char** v) { void (Fl_Widget::*foo)() = &Fl_Widget::set_simple_keyboard; return 0; }" HAVE_FLTK_DEAD_KEYS)

  # FLTK STR #2636
  check_cxx_source_compiles("#include <FL/Fl.H>\nint main(int c, char** v) { Fl::add_clipboard_notify(NULL, NULL); return 0; }" HAVE_FLTK_CLIPBOARD)

  # FLTK STR #2638
  check_cxx_source_compiles("#include <FL/Enumerations.H>\nint main(int c, char** v) { return FL_Volume_Down; }" HAVE_FLTK_MEDIAKEYS)

  # FLTK STR #2641
  check_cxx_source_compiles("#include <FL/Enumerations.H>\nint main(int c, char** v) { return FL_FULLSCREEN; }" HAVE_FLTK_FULLSCREEN)

  # FLTK STR #2660
  check_cxx_source_compiles("#include <FL/Fl_Window.H>\nint main(int c, char** v) { void (Fl_Window::*foo)(const Fl_RGB_Image*,int,int) = &Fl_Window::cursor; return 0; }" HAVE_FLTK_CURSOR)

  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()

# Check for GNUTLS library
option(ENABLE_GNUTLS "Enable protocol encryption and advanced authentication" ON)
if(ENABLE_GNUTLS)
  find_package(GnuTLS)
  if (GNUTLS_FOUND)
    include_directories(${GNUTLS_INCLUDE_DIR})
    add_definitions("-DHAVE_GNUTLS")
    add_definitions(${GNUTLS_DEFINITIONS})
    
    # Detect old version of GnuTLS
    set(CMAKE_REQUIRED_FLAGS -I${GNUTLS_INCLUDE_DIR})
    set(CMAKE_EXTRA_INCLUDE_FILES gnutls/gnutls.h)
    set(CMAKE_REQUIRED_LIBRARIES ${GNUTLS_LIBRARIES})
    if(WIN32)
      set(CMAKE_EXTRA_INCLUDE_FILES gcrypt.h ${CMAKE_EXTRA_INCLUDE_FILES})
      set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ws2_32 user32)
    endif()
    if(ZLIB_FOUND)
      # When we build against the static version of GnuTLS, we also use the
      # included version of Zlib, but it isn't built yet, so we have to use the
      # system's version (if available) to perform this test.
      set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES};-lz)
    endif()
    check_function_exists(gnutls_transport_set_global_errno HAVE_OLD_GNUTLS)
    check_function_exists(gnutls_x509_crt_print HAVE_GNUTLS_X509_CRT_PRINT)
    check_type_size(gnutls_x509_crt_t GNUTLS_X509_CRT_T)
    check_type_size(gnutls_datum_t GNUTLS_DATUM_T)
    check_type_size(gnutls_pk_algorithm_t GNUTLS_PK_ALGORITHM_T)
    check_type_size(gnutls_sign_algorithm_t GNUTLS_SIGN_ALGORITHM_T)
    set(CMAKE_REQUIRED_FLAGS)
    set(CMAKE_EXTRA_INCLUDE_FILES) 
    set(CMAKE_REQUIRED_LIBRARIES)
  endif()
endif()

# Check for PAM library
option(ENABLE_PAM "Enable PAM authentication support" ON)
if(ENABLE_PAM)
  check_include_files(security/pam_appl.h HAVE_PAM_H)
  set(CMAKE_REQUIRED_LIBRARIES -lpam)
  check_function_exists(pam_start HAVE_PAM_START)
  set(CMAKE_REQUIRED_LIBRARIES)
  if(HAVE_PAM_H AND HAVE_PAM_START)
    set(PAM_LIBS pam)
  else()
    set(ENABLE_PAM 0)
  endif()
endif()
set(HAVE_PAM ${ENABLE_PAM})

# Check for socket functions
if(WIN32)
  set(CMAKE_EXTRA_INCLUDE_FILES winsock2.h ws2tcpip.h)
  set(CMAKE_REQUIRED_LIBRARIES ws2_32)
else()
  set(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
endif()
check_function_exists(inet_aton HAVE_INET_ATON)
check_function_exists(inet_ntop HAVE_INET_NTOP)
check_type_size(socklen_t SOCKLEN_T)
set(CMAKE_EXTRA_INCLUDE_FILES) 
set(CMAKE_REQUIRED_LIBRARIES)

# Check for the newer standard string functions
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(vsnprintf HAVE_VSNPRINTF)

# Generate config.h and make sure the source finds it
configure_file(config.h.cmake.in config.h)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_BINARY_DIR})

add_subdirectory(common)

if(WIN32)
  add_subdirectory(win)
else()
  # No interest in building x related parts on Apple
  if(NOT APPLE)
    add_subdirectory(unix)
  endif()
endif()

if(BUILD_NEW_VNCVIEWER)
  if(ENABLE_NLS)
    add_subdirectory(po)
  endif()
  add_subdirectory(vncviewer)
endif()
