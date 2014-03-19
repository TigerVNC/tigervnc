# - Find gcrypt
# Find the native GCRYPT includes and library
#
#  GCRYPT_FOUND - True if gcrypt found.
#  GCRYPT_INCLUDE_DIR - where to find gcrypt.h, etc.
#  GCRYPT_LIBRARIES - List of libraries when using gcrypt.

if (GCRYPT_INCLUDE_DIR AND GCRYPT_LIBRARIES)
  set(GCRYPT_FIND_QUIETLY TRUE)
endif (GCRYPT_INCLUDE_DIR AND GCRYPT_LIBRARIES)

# Include dir
find_path(GCRYPT_INCLUDE_DIR
	NAMES
	  gcrypt.h
)

# Library
find_library(GCRYPT_LIBRARY 
  NAMES gcrypt
)

# handle the QUIETLY and REQUIRED arguments and set GCRYPT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GCRYPT DEFAULT_MSG GCRYPT_LIBRARY GCRYPT_INCLUDE_DIR)

IF(GCRYPT_FOUND)
  SET( GCRYPT_LIBRARIES ${GCRYPT_LIBRARY} )
ELSE(GCRYPT_FOUND)
  SET( GCRYPT_LIBRARIES )
ENDIF(GCRYPT_FOUND)

# Lastly make it so that the GCRYPT_LIBRARY and GCRYPT_INCLUDE_DIR variables
# only show up under the advanced options in the gui cmake applications.
MARK_AS_ADVANCED( GCRYPT_LIBRARY GCRYPT_INCLUDE_DIR )
