#
# Setup
#

cmake_minimum_required(VERSION 2.6)

project(TigerVNC)
set(VERSION 1.0.90)

# The RC version must always be four comma-separated numbers
set(RCVERSION 1,0,90,0)

if(MINGW OR CYGWIN)
  execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE BUILD)
  string(REGEX REPLACE "\n" "" BUILD ${BUILD})
elseif(WIN32)
  execute_process(COMMAND "${CMAKE_SOURCE_DIR}/cmakescripts/getdate.bat"
    OUTPUT_VARIABLE BUILD)
  string(REGEX REPLACE "\n" "" BUILD ${BUILD})
else()
  message(FATAL_ERROR "Platform not supported by this build system.  Use autotools instead.")
endif()

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

# Detect whether compiler is 64-bit
if((MSVC AND CMAKE_CL_64) OR (CMAKE_SIZEOF_VOID_P MATCHES 8))
  set(64BIT 1)
  set(WIN64 1)
endif()

if(64BIT)
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

# Check for zlib
find_package(ZLIB)
option(USE_INCLUDED_ZLIB "Force use of the bundled zlib")
if(NOT ZLIB_FOUND)
  message(STATUS "System zlib not found. Using included zlib")
  set(USE_INCLUDED_ZLIB 1)
endif()

# Check for libjpeg
find_package(JPEG REQUIRED)

# Check for GNUTLS library
find_package(GnuTLS)
if(GNUTLS_FOUND)
  include_directories(${GNUTLS_INCLUDE_DIR})
  add_definitions("-DHAVE_GNUTLS")
  add_definitions(${GNUTLS_DEFINITIONS})
endif()

# Generate config.h
include(CheckIncludeFiles)
include(CheckFunctionExists)
set(CMAKE_EXTRA_INCLUDE_FILES winsock2.h)
set(CMAKE_REQUIRED_LIBRARIES ws2_32)
check_function_exists(inet_aton HAVE_INET_ATON)
check_function_exists(inet_ntop HAVE_INET_NTOP)
set(CMAKE_EXTRA_INCLUDE_FILES) 
set(CMAKE_REQUIRED_LIBRARIES)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
configure_file(config.h.cmake.in config.h)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_BINARY_DIR})

add_definitions(-D_WIN32_IE=0x0500 -D_WIN32_WINNT=0x0500)

add_subdirectory(common)
add_subdirectory(win)
