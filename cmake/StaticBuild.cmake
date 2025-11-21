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

option(BUILD_STATIC_GCC
    "Link statically against only libgcc and libstdc++" OFF)

if(BUILD_STATIC)
  message(STATUS "Attempting to link static binaries...")

  set(BUILD_STATIC_GCC 1)

  set(JPEG_LIBRARIES "-Wl,-Bstatic -ljpeg -Wl,-Bdynamic")
  set(ZLIB_LIBRARIES "-Wl,-Bstatic -lz -Wl,-Bdynamic")
  set(PIXMAN_LIBRARIES "-Wl,-Bstatic -lpixman-1 -Wl,-Bdynamic")

  # FIXME: This code has not yet been tested
  if(UNIX AND NOT APPLE)
    if (PIPEWIRE_FOUND)
      set(PIPEWIRE_LIBRARIES, "-Wl,-Bstatic -lpipewire-0.3 -Wl, -Bdynamic")
    endif()
    if (UUID_FOUND)
      set(UUID_LIBRARIES, "-Wl,-Bstatic -luuid -Wl, -Bdynamic")
    endif()
    if (GLIB_FOUND)
      set(GLIB_LIBRARIES, "-Wl,-Bstatic -lglib-2.0 -Wl, -Bdynamic")
    endif()
    if (GIO_FOUND)
      set(GIO_LIBRARIES, "-Wl,-Bstatic  -lgio-2.0 -lgobject-2.0 -lglib-2.0 -Wl, -Bdynamic")
    endif()
    if (GOBJECT_FOUND)
      set(GOBJECT_LIBRARIES, "-Wl,-Bstatic -lgobject-2.0 -lglib-2.0 -Wl, -Bdynamic")
    endif()
  endif()

  # gettext is included in libc on many unix systems
  check_function_exists(dgettext LIBC_HAS_DGETTEXT)
  if(NOT LIBC_HAS_DGETTEXT)
    FIND_LIBRARY(UNISTRING_LIBRARY NAMES unistring libunistring)

    set(Intl_LIBRARIES "-Wl,-Bstatic -lintl")

    if(UNISTRING_LIBRARY)
      set(Intl_LIBRARIES "${Intl_LIBRARIES} -lunistring")
    endif()

    set(Intl_LIBRARIES "${Intl_LIBRARIES} -Wl,-Bdynamic")

    if(APPLE)
      set(Intl_LIBRARIES "${Intl_LIBRARIES} -liconv")
    else()
      set(Intl_LIBRARIES "${Intl_LIBRARIES} -Wl,-Bstatic")
      set(Intl_LIBRARIES "${Intl_LIBRARIES} -liconv")
      set(Intl_LIBRARIES "${Intl_LIBRARIES} -Wl,-Bdynamic")
    endif()

    if(APPLE)
      set(Intl_LIBRARIES "${Intl_LIBRARIES} -framework Carbon")
    endif()
  endif()

  if(GNUTLS_FOUND)
    # GnuTLS has historically had different crypto backends
    FIND_LIBRARY(NETTLE_LIBRARY NAMES nettle libnettle
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(TASN1_LIBRARY NAMES tasn1 libtasn1
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(IDN2_LIBRARY NAMES idn2 libidn2
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(ZSTD_LIBRARY NAMES zstd libzstd
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})

    set(GNUTLS_LIBRARIES "-Wl,-Bstatic -lgnutls")

    if(TASN1_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -ltasn1")
    endif()
    if(NETTLE_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lhogweed -lnettle -lgmp")
    endif()
    if(IDN2_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lidn2")
    endif()
    if(ZSTD_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lzstd")
    endif()

    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -Wl,-Bdynamic")

    if (WIN32)
      FIND_LIBRARY(P11KIT_LIBRARY NAMES p11-kit libp11-kit
        HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
      FIND_LIBRARY(UNISTRING_LIBRARY NAMES unistring libunistring
        HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})

      # GnuTLS uses various crypto-api stuff
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lcrypt32 -lncrypt -lbcrypt")
      # And sockets
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lws2_32")

      # p11-kit only available as dynamic library for MSYS2 on Windows and dynamic linking of unistring is required
      if(P11KIT_LIBRARY)
        set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lp11-kit")
      endif()
      if(UNISTRING_LIBRARY)
        set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lunistring")
      endif()
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
      # nanosleep() lives here on Solaris
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lrt")
      # and socket functions are hidden here
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lsocket")
    endif()

    # GnuTLS uses gettext and zlib, so make sure those are always
    # included and in the proper order
    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} ${ZLIB_LIBRARIES}")
    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} ${Intl_LIBRARIES}")

    # The last variables might introduce whitespace, which CMake
    # throws a hissy fit about
    string(STRIP ${GNUTLS_LIBRARIES} GNUTLS_LIBRARIES)
  endif()

  if(NETTLE_FOUND)
    set(NETTLE_LIBRARIES "-Wl,-Bstatic -lnettle -Wl,-Bdynamic")
    set(HOGWEED_LIBRARIES "-Wl,-Bstatic -lhogweed -lnettle -lgmp -Wl,-Bdynamic")
  endif()

  if(DEFINED FLTK_LIBRARIES)
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
        set(FLTK_LIBRARIES "${FLTK_LIBRARIES} -Wl,-Bstatic -lXcursor -lXfixes -lXft -lfontconfig -lexpat -lfreetype -lpng -lbz2 -luuid -lXrender -lXext -lXinerama -Wl,-Bdynamic")
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
    if(X11_Xrandr_LIB)
      set(X11_Xrandr_LIB "-Wl,-Bstatic -lXrandr -lXrender -Wl,-Bdynamic")
    endif()
    if(X11_Xi_LIB)
      set(X11_Xi_LIB "-Wl,-Bstatic -lXi -Wl,-Bdynamic")
    endif()
  endif()
endif()

if(BUILD_STATIC_GCC)
  set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -static-libgcc")
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libstdc++ -static-libgcc")
  if(ENABLE_ASAN)
    set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -static-libasan")
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libasan")
  endif()
  if(ENABLE_TSAN)
    set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -static-libtsan")
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -static-libtsan")
  endif()
endif()
