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
include(CheckCSourceRuns)

include(CMakeMacroLibtoolFile)

project(tigervnc)
set(VERSION 1.4.80)

# The RC version must always be four comma-separated numbers
set(RCVERSION 1,4,80,0)

# Installation paths
set(BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(DATA_DIR "${CMAKE_INSTALL_PREFIX}/share")
set(MAN_DIR "${DATA_DIR}/man")
set(LOCALE_DIR "${DATA_DIR}/locale")
set(DOC_DIR "${CMAKE_INSTALL_PREFIX}/share/doc/${CMAKE_PROJECT_NAME}-${VERSION}")

if(WIN32)
set(BIN_DIR "${CMAKE_INSTALL_PREFIX}")
set(DOC_DIR "${CMAKE_INSTALL_PREFIX}")
endif()

if(MSVC)
  message(FATAL_ERROR "TigerVNC cannot be built with Visual Studio.  Please use MinGW")
endif()

set(BUILD_TIMESTAMP "")
execute_process(COMMAND "date" "+%Y-%m-%d %H:%M" OUTPUT_VARIABLE BUILD_TIMESTAMP)

if(NOT BUILD_TIMESTAMP)
  set(BUILD_TIMESTAMP "")
else()
  string(REGEX REPLACE "\n" "" BUILD_TIMESTAMP ${BUILD_TIMESTAMP})
endif()

# Default to optimised builds instead of debug ones. Our code has no bugs ;)
# (CMake makes it fairly easy to toggle this back to Debug if needed)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

message(STATUS "VERSION = ${VERSION}")
add_definitions(-DBUILD_TIMESTAMP="${BUILD_TIMESTAMP}")

# We want to keep our asserts even in release builds so remove NDEBUG
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -UNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -UNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -UNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -UNDEBUG")

if(NOT DEFINED BUILD_WINVNC)
  set(BUILD_WINVNC 1)
endif()

# Minimum version is Windows 2000 (5.0)
if(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P MATCHES 8)
    add_definitions(-D_WIN32_IE=0x0500 -D_WIN32_WINNT=0x0500)
  else()
    set(WIN64 1)
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

# MinGW64 has header support but no library support for IActiveDesktop, so we
# need to check for both the header and library and use our own implementation
# in common/os if either doesn't exist.
if(WIN32)
  check_c_source_compiles("#include <windows.h>\n#include <wininet.h>\n#include <shlobj.h>\nint main(int c, char** v) {IActiveDesktop iad; return 0;}" HAVE_ACTIVE_DESKTOP_H)
  check_c_source_compiles("#include <windows.h>\n#include <wininet.h>\n#include <shlobj.h>\nint main(int c, char** v) {GUID i = CLSID_ActiveDesktop; return 0;}" HAVE_ACTIVE_DESKTOP_L)
endif()

# X11 stuff. It's in a if() so that we can say REQUIRED
if(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
endif()

# Check for zlib
find_package(ZLIB)
option(USE_INCLUDED_ZLIB "Force use of the bundled zlib")
if(NOT ZLIB_FOUND)
  set(USE_INCLUDED_ZLIB 1)
endif()
if(USE_INCLUDED_ZLIB)
  message(STATUS "Using included zlib library")
endif()

# Check for gettext
option(ENABLE_NLS "Enable translation of program messages" ON)
if(ENABLE_NLS)
  # Tools
  find_package(Gettext)

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

  if(NOT GETTEXT_FOUND OR NOT ICONV_FOUND)
    message(WARNING "Gettext NOT found.  Native Language Support disabled.")
    set(ENABLE_NLS 0)
  endif()
endif()

# Check for libjpeg
find_package(JPEG REQUIRED)

# Warn if it doesn't seem to be the accelerated libjpeg that's found
set(CMAKE_REQUIRED_LIBRARIES ${JPEG_LIBRARIES})
set(CMAKE_REQUIRED_FLAGS -I${JPEG_INCLUDE_DIR})

set(JPEG_TEST_SOURCE "\n
  #include <stdio.h>\n
  #include <jpeglib.h>\n
  int main(void) {\n
    struct jpeg_compress_struct cinfo;\n
    struct jpeg_error_mgr jerr;\n
    cinfo.err=jpeg_std_error(&jerr);\n
    jpeg_create_compress(&cinfo);\n
    cinfo.input_components = 3;\n
    jpeg_set_defaults(&cinfo);\n
    cinfo.in_color_space = JCS_EXT_RGB;\n
    jpeg_default_colorspace(&cinfo);\n
    return 0;\n
  }")

if(CMAKE_CROSSCOMPILING)
  check_c_source_compiles("${JPEG_TEST_SOURCE}" FOUND_LIBJPEG_TURBO)
else()
  check_c_source_runs("${JPEG_TEST_SOURCE}" FOUND_LIBJPEG_TURBO)
endif()

set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_FLAGS)
set(CMAKE_REQUIRED_DEFINITIONS)

if(NOT FOUND_LIBJPEG_TURBO)
  message(STATUS "WARNING: You are not using libjpeg-turbo. Performance will suffer.")
endif()

option(BUILD_JAVA "Build Java version of the TigerVNC Viewer" FALSE)
if(BUILD_JAVA)
  add_subdirectory(java)
endif()

# Check for FLTK
set(FLTK_SKIP_FLUID TRUE)
set(FLTK_SKIP_OPENGL TRUE)
set(FLTK_SKIP_FORMS TRUE)
find_package(FLTK)

if(UNIX AND NOT APPLE)
  # No proper handling for extra X11 libs that FLTK might need...
  if(X11_Xft_FOUND)
    # Xft headers include references to fontconfig, so we need
    # to link to that as well
    find_library(FONTCONFIG_LIB fontconfig)
    set(FLTK_LIBRARIES ${FLTK_LIBRARIES} ${X11_Xft_LIB} ${FONTCONFIG_LIB})
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
  if(X11_Xrender_FOUND)
    set(FLTK_LIBRARIES ${FLTK_LIBRARIES} ${X11_Xrender_LIB})
  endif()
endif()

if(FLTK_FOUND)
  set(CMAKE_REQUIRED_INCLUDES ${FLTK_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${FLTK_LIBRARIES})

  # FLTK STR #2636
  check_cxx_source_compiles("#include <FL/Fl.H>\nint main(int c, char** v) { Fl::add_clipboard_notify(NULL, NULL); return 0; }" HAVE_FLTK_CLIPBOARD)

  # FLTK STR #2638
  check_cxx_source_compiles("#include <FL/Enumerations.H>\nint main(int c, char** v) { return FL_Volume_Down; }" HAVE_FLTK_MEDIAKEYS)

  # FLTK STR #2641
  check_cxx_source_compiles("#include <FL/Enumerations.H>\nint main(int c, char** v) { return FL_FULLSCREEN; }" HAVE_FLTK_FULLSCREEN)

  # FLTK STR #2660
  check_cxx_source_compiles("#include <FL/Fl_Window.H>\nint main(int c, char** v) { void (Fl_Window::*foo)(const Fl_RGB_Image*,int,int) = &Fl_Window::cursor; return 0; }" HAVE_FLTK_CURSOR)

  # FLTK STR #2697
  check_cxx_source_compiles("#include <FL/Fl.H>\nint main(int c, char** v) { int X, Y, W, H; Fl::screen_work_area(X, Y, W, H); return 0; }" HAVE_FLTK_WORK_AREA)

  # FLTK STR #2816
  check_cxx_source_compiles("#include <FL/Fl_Window.H>\nint main(int c, char** v) { Fl_Window::default_icons(0, 0); return 0; }" HAVE_FLTK_ICONS)

  # FLTK STR #2860
  check_cxx_source_compiles("#include <FL/Fl_Window.H>\nint main(int c, char** v) { void (Fl_Window::*foo)(int,int,int,int) = &Fl_Window::fullscreen_screens; return 0; }" HAVE_FLTK_FULLSCREEN_SCREENS)

  # FLTK STR #xxxx
  check_cxx_source_compiles("#include <FL/Fl.H>\nint main(int c, char** v) { Fl::add_system_handler(NULL, NULL); return 0; }" HAVE_FLTK_XHANDLERS)

  # FLTK STR #xxxx
  check_cxx_source_compiles("#include <FL/Fl.H>\nint main(int c, char** v) { Fl::disable_im(); return 0; }" HAVE_FLTK_IM)

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
      set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ws2_32 user32)
    endif()
    if(ZLIB_FOUND)
      # When we build against the static version of GnuTLS, we also use the
      # included version of Zlib, but it isn't built yet, so we have to use the
      # system's version (if available) to perform this test.
      set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES};-lz)
    endif()
    check_function_exists(gnutls_transport_set_errno HAVE_GNUTLS_SET_ERRNO)
    check_function_exists(gnutls_transport_set_global_errno HAVE_GNUTLS_SET_GLOBAL_ERRNO)
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
# This might give a false positive on Windows as it is also guarded by
# a version check that we do not satisfy (requires Vista, but we target
# Windows 2000).
if(NOT WIN32)
check_function_exists(inet_pton HAVE_INET_PTON)
endif()
check_function_exists(getaddrinfo HAVE_GETADDRINFO)
set(CMAKE_EXTRA_INCLUDE_FILES) 
set(CMAKE_REQUIRED_LIBRARIES)

# Generate config.h and make sure the source finds it
configure_file(config.h.in config.h)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_BINARY_DIR})

include(cmake/StaticBuild.cmake)

add_subdirectory(common)

if(WIN32)
  add_subdirectory(win)
else()
  # No interest in building x related parts on Apple
  if(NOT APPLE)
    add_subdirectory(unix)
  endif()
endif()

if(ENABLE_NLS)
  add_subdirectory(po)
endif()

option(BUILD_VIEWER "Build TigerVNC viewer" ON)
if(BUILD_VIEWER)
  add_subdirectory(vncviewer)
  add_subdirectory(media)
endif()

add_subdirectory(tests)


include(cmake/BuildPackages.cmake)

# uninstall
configure_file("${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "cmake_uninstall.cmake" IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P cmake_uninstall.cmake)
