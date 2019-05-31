# From: http://gitorious.org/gammu/mainline/blobs/master/cmake/FindIconv.cmake

# - Try to find Iconv 
# Once done this will define 
# 
#  ICONV_FOUND - system has Iconv 
#  ICONV_INCLUDE_DIR - the Iconv include directory 
#  ICONV_LIBRARIES - Link these to use Iconv 
#  ICONV_SECOND_ARGUMENT_IS_CONST - the second argument for iconv() is const
# 
include(CheckCCompilerFlag)
include(CheckCXXSourceCompiles)

IF (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
  # Already in cache, be silent
  SET(ICONV_FIND_QUIETLY TRUE)
ENDIF (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)

FIND_PATH(ICONV_INCLUDE_DIR iconv.h) 
 
FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv libiconv c)
 
IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES) 
   SET(ICONV_FOUND TRUE) 
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES) 

set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
IF(ICONV_FOUND)
  check_c_compiler_flag("-Werror" ICONV_HAVE_WERROR)
  set (CMAKE_C_FLAGS_BACKUP "${CMAKE_C_FLAGS}")
  if(ICONV_HAVE_WERROR)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla")
  endif(ICONV_HAVE_WERROR)
  check_c_source_compiles("
  #include <iconv.h>
  int main(){
    iconv_t conv = 0;
    const char* in = 0;
    size_t ilen = 0;
    char* out = 0;
    size_t olen = 0;
    iconv(conv, &in, &ilen, &out, &olen);
    return 0;
  }
" ICONV_SECOND_ARGUMENT_IS_CONST )
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS_BACKUP}")
ENDIF(ICONV_FOUND)
set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

IF(ICONV_FOUND) 
  IF(NOT ICONV_FIND_QUIETLY) 
    MESSAGE(STATUS "Found Iconv: ${ICONV_LIBRARIES}") 
  ENDIF(NOT ICONV_FIND_QUIETLY) 
ELSE(ICONV_FOUND) 
  IF(Iconv_FIND_REQUIRED) 
    MESSAGE(FATAL_ERROR "Could not find Iconv") 
  ENDIF(Iconv_FIND_REQUIRED) 
ENDIF(ICONV_FOUND) 

MARK_AS_ADVANCED(
  ICONV_INCLUDE_DIR
  ICONV_LIBRARIES
  ICONV_SECOND_ARGUMENT_IS_CONST
)
