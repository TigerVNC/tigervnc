#
# Setup
#

cmake_minimum_required(VERSION 3.10.0)

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

include(cmake/TargetLinkDirectories.cmake)

project(tigervnc)
set(VERSION 1.12.80)

# The RC version must always be four comma-separated numbers
string(REPLACE . , RCVERSION "${VERSION}.0")

# Installation paths
include(GNUInstallDirs)
set(CMAKE_INSTALL_UNITDIR "lib/systemd/system" CACHE PATH "systemd unit files (lib/systemd/system)")
if(IS_ABSOLUTE "${CMAKE_INSTALL_UNITDIR}")
  set(CMAKE_INSTALL_FULL_UNITDIR "${CMAKE_INSTALL_UNITDIR}")
else()
  set(CMAKE_INSTALL_FULL_UNITDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_UNITDIR}")
endif()

option(INSTALL_SYSTEMD_UNITS "Install TigerVNC systemd units" ON)

if(MSVC)
  message(FATAL_ERROR "TigerVNC cannot be built with Visual Studio.  Please use MinGW")
endif()

if(NOT BUILD_TIMESTAMP)
  STRING(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M" UTC)
endif()

# Default to optimised builds instead of debug ones. Our code has no bugs ;)
# (CMake makes it fairly easy to toggle this back to Debug if needed)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

message(STATUS "VERSION = ${VERSION}")
message(STATUS "BUILD_TIMESTAMP = ${BUILD_TIMESTAMP}")
add_definitions(-DBUILD_TIMESTAMP="${BUILD_TIMESTAMP}")

# We want to keep our asserts even in release builds so remove NDEBUG
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -UNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -UNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -UNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -UNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -UNDEBUG")

# But extra debug checks are still gated by this custom define
IF(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-D_DEBUG)
ENDIF()

# Make sure we get a sane C version
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

# Tell the compiler to be stringent
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wformat=2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wformat=2")
# Make sure we catch these issues whilst developing
IF(CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Werror=vla")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Werror=vla")
ENDIF()
# clang doesn't support format_arg, which breaks this warning
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-nonliteral -Wno-format-security")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-nonliteral -Wno-format-security")
endif()

option(ENABLE_ASAN "Enable address sanitizer support" OFF)
if(ENABLE_ASAN AND NOT WIN32 AND NOT APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

option(ENABLE_TSAN "Enable thread sanitizer support" OFF)
if(ENABLE_TSAN AND NOT WIN32 AND NOT APPLE AND CMAKE_SIZEOF_VOID_P MATCHES 8)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

if(NOT DEFINED BUILD_WINVNC)
  set(BUILD_WINVNC 1)
endif()

# Minimum version is Windows 7
if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Legacy macros (macOS 10.12 and older) conflict with our code
if(APPLE)
  add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
endif()

# MinGW64 has header support but no library support for IActiveDesktop, so we
# need to check for both the header and library and use our own implementation
# in common/os if either doesn't exist.
if(WIN32)
  check_c_source_compiles("#include <windows.h>\n#include <wininet.h>\n#include <shlobj.h>\nint main(int c, char** v) {IActiveDesktop iad; (void)iad; return 0;}" HAVE_ACTIVE_DESKTOP_H)
  check_c_source_compiles("#include <windows.h>\n#include <wininet.h>\n#include <shlobj.h>\nint main(int c, char** v) {GUID i = CLSID_ActiveDesktop; (void)i; return 0;}" HAVE_ACTIVE_DESKTOP_L)
endif()

# X11 stuff. It's in a if() so that we can say REQUIRED
if(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
endif()

# Check for zlib
find_package(ZLIB REQUIRED)

# Check for pixman
find_package(Pixman REQUIRED)

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
      set(CMAKE_REQUIRED_FLAGS -fno-builtin-dgettext)
      check_function_exists(dgettext LIBC_HAS_DGETTEXT)
      if(LIBC_HAS_DGETTEXT)
        set(GETTEXT_FOUND TRUE)
      else()
        find_library(LIBINTL_LIBRARY NAMES intl libintl)
        if(LIBINTL_LIBRARY)
          check_library_exists(${LIBINTL_LIBRARY} "dgettext" "" LIBINTL_HAS_DGETTEXT)
          if(LIBINTL_HAS_DGETTEXT)
            set(GETTEXT_LIBRARIES ${LIBINTL_LIBRARY} ${ICONV_LIBRARIES})
            set(GETTEXT_FOUND TRUE)
          endif()
        endif()
      endif()
      set(CMAKE_REQUIRED_LIBRARIES)
      set(CMAKE_REQUIRED_FLAGS)
    endif()
  endif()

  if(NOT GETTEXT_FOUND OR NOT ICONV_FOUND)
    message(WARNING "Gettext NOT found.  Native Language Support disabled.")
    set(ENABLE_NLS 0)
  endif()
endif()

option(ENABLE_H264 "Enable H.264 RFB encoding" ON)
if(ENABLE_H264)
  if(WIN32)
    add_definitions("-DHAVE_H264")
    set(H264_LIBS "WIN")  # may be LIBAV in the future
    set(H264_LIBRARIES ole32 mfplat mfuuid wmcodecdspuuid)
  else()
    find_package(Ffmpeg)
    if (AVCODEC_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND)
      set(H264_INCLUDE_DIRS ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS})
      set(H264_LIBRARIES ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})
      set(H264_LIBRARY_DIRS ${AVCODEC_LIBRARY_DIRS} ${AVUTIL_LIBRARY_DIRS} ${SWSCALE_LIBRARY_DIRS})
      add_definitions("-D__STDC_CONSTANT_MACROS")
      add_definitions("-DHAVE_H264")
      set(H264_LIBS "LIBAV")
    else()
      set(H264_LIBS "NONE")
      message(WARNING "FFMPEG support can't be found")
    endif()
  endif()
  add_definitions("-DH264_${H264_LIBS}")
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

option(BUILD_VIEWER "Build TigerVNC viewer" ON)
if(BUILD_VIEWER)
  # Check for FLTK
  set(FLTK_SKIP_FLUID TRUE)
  set(FLTK_SKIP_OPENGL TRUE)
  set(FLTK_SKIP_FORMS TRUE)
  find_package(FLTK REQUIRED)
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
endif()

# Check for GNUTLS library
option(ENABLE_GNUTLS "Enable protocol encryption and advanced authentication" ON)
if(ENABLE_GNUTLS)
  find_package(GnuTLS)
  if (GNUTLS_FOUND)
    add_definitions("-DHAVE_GNUTLS")
  endif()
endif()

option(ENABLE_NETTLE "Enable RSA-AES security types" ON)
if (ENABLE_NETTLE)
  find_package(Nettle)
  if (NETTLE_FOUND)
    add_definitions("-DHAVE_NETTLE")
  endif()
endif()

# Check for PAM library
if(UNIX AND NOT APPLE)
  check_include_files(security/pam_appl.h HAVE_PAM_H)
  set(CMAKE_REQUIRED_LIBRARIES -lpam)
  check_function_exists(pam_start HAVE_PAM_START)
  set(CMAKE_REQUIRED_LIBRARIES)
  if(HAVE_PAM_H AND HAVE_PAM_START)
    set(PAM_LIBS pam)
  else()
    message(FATAL_ERROR "Could not find PAM development files")
  endif()
endif()

# Check for SELinux library
if(UNIX AND NOT APPLE)
  check_include_files(selinux/selinux.h HAVE_SELINUX_H)
  if(HAVE_SELINUX_H)
    set(CMAKE_REQUIRED_LIBRARIES -lselinux)
    set(CMAKE_REQUIRED_LIBRARIES)
    set(SELINUX_LIBS selinux)
    add_definitions("-DHAVE_SELINUX")
  else()
    message(WARNING "Could not find SELinux development files")
  endif()
endif()

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

if(BUILD_VIEWER)
  add_subdirectory(vncviewer)
  add_subdirectory(media)
endif()

add_subdirectory(tests)


if(BUILD_VIEWER)
  add_subdirectory(release)
endif()

# uninstall
configure_file("${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "cmake_uninstall.cmake" IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P cmake_uninstall.cmake)
