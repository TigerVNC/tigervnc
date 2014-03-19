# - Find gpg-error
# Find the native GPG_ERROR includes and library
#
#  GPG_ERROR_FOUND - True if gpg-error found.
#  GPG_ERROR_INCLUDE_DIR - where to find gpg-error.h, etc.
#  GPG_ERROR_LIBRARIES - List of libraries when using gpg-error.

if (GPG_ERROR_INCLUDE_DIR AND GPG_ERROR_LIBRARIES)
  set(GPG_ERROR_FIND_QUIETLY TRUE)
endif (GPG_ERROR_INCLUDE_DIR AND GPG_ERROR_LIBRARIES)

# Include dir
find_path(GPG_ERROR_INCLUDE_DIR
	NAMES
	  gpg-error.h
)

# Library
find_library(GPG_ERROR_LIBRARY 
  NAMES gpg-error
)

# handle the QUIETLY and REQUIRED arguments and set GPG_ERROR_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GPG_ERROR DEFAULT_MSG GPG_ERROR_LIBRARY GPG_ERROR_INCLUDE_DIR)

IF(GPG_ERROR_FOUND)
  SET( GPG_ERROR_LIBRARIES ${GPG_ERROR_LIBRARY} )
ELSE(GPG_ERROR_FOUND)
  SET( GPG_ERROR_LIBRARIES )
ENDIF(GPG_ERROR_FOUND)

# Lastly make it so that the GPG_ERROR_LIBRARY and GPG_ERROR_INCLUDE_DIR variables
# only show up under the advanced options in the gui cmake applications.
MARK_AS_ADVANCED( GPG_ERROR_LIBRARY GPG_ERROR_INCLUDE_DIR )
