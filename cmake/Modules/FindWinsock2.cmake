# - Find ws2_32 functions
# This module finds ws2_32 libraries.
#
# It sets the following variables:
#  WINSOCK2_FOUND       - Set to false, or undefined, if ws2_32 libraries aren't found.
#  WINSOCK2_LIBRARIES   - The ws2_32 library to link against.
#  WINSOCK2_INCLUDE_DIR - Path to winsock2.h, ws2tcpip.h

INCLUDE(CheckFunctionExists)

if (WINSOCK2_LIBRARIES)
  set(WINSOCK2_FIND_QUIETLY TRUE)
endif (WINSOCK2_LIBRARIES)

# Include dir
find_path(WINSOCK2_INCLUDE_DIR
  NAMES
    winsock2.h
    ws2tcpip.h
)

FIND_LIBRARY(WINSOCK2_LIBRARY NAMES ws2_32)

IF (WINSOCK2_LIBRARY)
  SET(WINSOCK2_FOUND TRUE)
ELSE (WINSOCK2_LIBRARY)
  MESSAGE(FATAL_ERROR "Could not find ws2_32")
ENDIF (WINSOCK2_LIBRARY)

IF (WINSOCK2_FOUND)
  SET(WINSOCK2_LIBRARIES ${WINSOCK2_LIBRARY})
  # show which ws2_32 was found only if not quiet
  IF (NOT WINSOCK2_FIND_QUIETLY)
    MESSAGE(STATUS "Found ws2_32: ${WINSOCK2_LIBRARY}")
  ENDIF (NOT WINSOCK2_FIND_QUIETLY)
ENDIF (WINSOCK2_FOUND)

MARK_AS_ADVANCED(WINSOCK2_LIBRARY WINSOCK2_INCLUDE_DIR)
