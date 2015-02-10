#
# Best-effort magic that tries to produce semi-static binaries
# (i.e. only depends on "safe" libraries like libc and libX11)
#
# Note that this often fails as there is no way to automatically
# determine the dependencies of the libraries we depend on, and
# a lot of details change with each different build environment.
#

option(BUILD_STATIC
    "Link statically against most libraries, if possible" OFF)

if(BUILD_STATIC)
  message(STATUS "Attempting to link static binaries...")

  set(JPEG_LIBRARIES "-Wl,-Bstatic -ljpeg -Wl,-Bdynamic")

  if(WIN32 AND NOT USE_INCLUDED_ZLIB)
    set(ZLIB_LIBRARIES "-Wl,-Bstatic -lz -Wl,-Bdynamic")
  endif()

  # gettext is included in libc on many unix systems
  if(NOT LIBC_HAS_DGETTEXT)
    set(GETTEXT_LIBRARIES "-Wl,-Bstatic -lintl -liconv -Wl,-Bdynamic")
  endif()

  if(GNUTLS_FOUND)
    # GnuTLS has historically had different crypto backends
    FIND_LIBRARY(GCRYPT_LIBRARY NAMES gcrypt libgcrypt
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(NETTLE_LIBRARY NAMES nettle libnettle
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(TASN1_LIBRARY NAMES tasn1 libtasn1
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})

    set(GNUTLS_LIBRARIES "-Wl,-Bstatic -lgnutls")

    if(TASN1_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -ltasn1")
    endif()
    if(NETTLE_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lnettle -lhogweed -lgmp")
    endif()
    if(GCRYPT_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lgcrypt -lgpg-error")
    endif()

    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -Wl,-Bdynamic")

    # GnuTLS uses various crypto-api stuff
    if (WIN32)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lcrypt32")
    endif()

    # nanosleep() lives here on Solaris
    if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lrt")
    endif()

    # GnuTLS uses gettext and zlib, so make sure those are always
    # included and in the proper order
    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} ${ZLIB_LIBRARIES}")
    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} ${GETTEXT_LIBRARIES}")

    # The last variables might introduce whitespace, which CMake
    # throws a hissy fit about
    string(STRIP ${GNUTLS_LIBRARIES} GNUTLS_LIBRARIES)
  endif()

  if(FLTK_FOUND)
    set(FLTK_LIBRARIES "-Wl,-Bstatic -lfltk_images -lpng -ljpeg -lfltk -Wl,-Bdynamic")

    if(WIN32)
      set(FLTK_LIBRARIES "${FLTK_LIBRARIES} -lcomctl32")
    elseif(APPLE)
      set(FLTK_LIBRARIES "${FLTK_LIBRARIES} -framework Cocoa")
    else()
      set(FLTK_LIBRARIES "${FLTK_LIBRARIES} -lm -ldl")
    endif()

    if(X11_FOUND AND NOT APPLE)
      if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
        set(FLTK_LIBRARIES "${FLTK_LIBRARIES} ${X11_Xcursor_LIB} ${X11_Xfixes_LIB} -Wl,-Bstatic -lXft -Wl,-Bdynamic -lfontconfig -lXrender -lXext -R/usr/sfw/lib -L=/usr/sfw/lib -lfreetype -lsocket -lnsl")
      else()
        set(FLTK_LIBRARIES "${FLTK_LIBRARIES} -Wl,-Bstatic -lXcursor -lXfixes -lXft -lfontconfig -lexpat -lfreetype -lbz2 -lXrender -lXext -lXinerama -Wl,-Bdynamic")
      endif()

      set(FLTK_LIBRARIES "${FLTK_LIBRARIES} -lX11")
    endif()
  endif()

  # X11 libraries change constantly on Linux systems so we have to link
  # them statically, even libXext. libX11 is somewhat stable, although
  # even it has had an ABI change once or twice.
  if(X11_FOUND AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
    set(X11_LIBRARIES "-Wl,-Bstatic -lXext -Wl,-Bdynamic -lX11")
    if(X11_XTest_LIB)
      set(X11_XTest_LIB "-Wl,-Bstatic -lXtst -Wl,-Bdynamic")
    endif()
    if(X11_Xdamage_LIB)
      set(X11_Xdamage_LIB "-Wl,-Bstatic -lXdamage -Wl,-Bdynamic")
    endif()
  endif()

  # This ensures that we don't depend on libstdc++ or libgcc_s
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nodefaultlibs")
  set(STATIC_BASE_LIBRARIES "-Wl,-Bstatic -lstdc++ -Wl,-Bdynamic")
  if(WIN32)
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -lmingw32 -lgcc_eh -lgcc -lmoldname -lmingwex -lmsvcrt")
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -luser32 -lkernel32 -ladvapi32 -lshell32")
    # mingw has some fun circular dependencies that requires us to link
    # these things again
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -lmingw32 -lgcc_eh -lgcc -lmoldname -lmingwex -lmsvcrt")
  else()
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -lgcc -lgcc_eh -lc")
  endif()
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} ${STATIC_BASE_LIBRARIES}")

endif()
