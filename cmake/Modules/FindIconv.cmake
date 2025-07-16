#[=======================================================================[.rst:
FindIconv
---------

Find the iconv libraries

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``ICONV_INCLUDE_DIRS``
  where to find iconv.h, etc.
``ICONV_LIBRARIES``
  the libraries to link against to use iconv.
``ICONV_FOUND``
  TRUE if found

#]=======================================================================]

find_path(Iconv_INCLUDE_DIR NAMES iconv.h)
mark_as_advanced(Iconv_INCLUDE_DIR)

find_library(Iconv_LIBRARY NAMES iconv libiconv c)
mark_as_advanced(Iconv_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iconv
  REQUIRED_VARS
    Iconv_LIBRARY Iconv_INCLUDE_DIR
)

if (Iconv_FOUND)
  set(ICONV_INCLUDE_DIRS ${Iconv_INCLUDE_DIR})
  set(ICONV_LIBRARIES ${Iconv_LIBRARY})
endif()
