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

  # gettext is included in libc on many unix systems
  if(NOT LIBC_HAS_DGETTEXT)
    FIND_LIBRARY(UNISTRING_LIBRARY NAMES unistring libunistring
      HINTS ${PC_GETTEXT_LIBDIR} ${PC_GETTEXT_LIBRARY_DIRS})
    FIND_LIBRARY(INTL_LIBRARY NAMES intl libintl
      HINTS ${PC_GETTEXT_LIBDIR} ${PC_GETTEXT_LIBRARY_DIRS})
    FIND_LIBRARY(ICONV_LIBRARY NAMES iconv libiconv
      HINTS ${PC_GETTEXT_LIBDIR} ${PC_GETTEXT_LIBRARY_DIRS})

    set(GETTEXT_LIBRARIES "-Wl,-Bstatic")

    if(INTL_LIBRARY)
      set(GETTEXT_LIBRARIES "${GETTEXT_LIBRARIES} -lintl")
    endif()
    
    if(ICONV_LIBRARY)
      set(GETTEXT_LIBRARIES "${GETTEXT_LIBRARIES} -liconv")
    endif()

    set(GETTEXT_LIBRARIES "${GETTEXT_LIBRARIES} -Wl,-Bdynamic")

    # FIXME: MSYS2 doesn't include a static version of this library, so
    #        we'll have to link it dynamically for now
    if(UNISTRING_LIBRARY)
      set(GETTEXT_LIBRARIES "${GETTEXT_LIBRARIES} -lunistring")
    endif()

    if(APPLE)
      set(GETTEXT_LIBRARIES "${GETTEXT_LIBRARIES} -framework Carbon")
    endif()
  endif()

  if(GNUTLS_FOUND)
    # GnuTLS has historically had different crypto backends
    FIND_LIBRARY(GCRYPT_LIBRARY NAMES gcrypt libgcrypt
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(NETTLE_LIBRARY NAMES nettle libnettle
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(TASN1_LIBRARY NAMES tasn1 libtasn1
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
    FIND_LIBRARY(IDN2_LIBRARY NAMES idn2 libidn2
      HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})

    set(GNUTLS_LIBRARIES "-Wl,-Bstatic -lgnutls")

    if(TASN1_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -ltasn1")
    endif()
    if(NETTLE_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lhogweed -lnettle -lgmp")
    endif()
    if(GCRYPT_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lgcrypt -lgpg-error")
    endif()
    if(IDN2_LIBRARY)
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lidn2")
    endif()

    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -Wl,-Bdynamic")

    if (WIN32)
      FIND_LIBRARY(P11KIT_LIBRARY NAMES p11-kit libp11-kit
        HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})
      FIND_LIBRARY(UNISTRING_LIBRARY NAMES unistring libunistring
        HINTS ${PC_GNUTLS_LIBDIR} ${PC_GNUTLS_LIBRARY_DIRS})

      # GnuTLS uses various crypto-api stuff
      set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} -lcrypt32 -lncrypt")
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
    set(GNUTLS_LIBRARIES "${GNUTLS_LIBRARIES} ${GETTEXT_LIBRARIES}")

    # The last variables might introduce whitespace, which CMake
    # throws a hissy fit about
    string(STRIP ${GNUTLS_LIBRARIES} GNUTLS_LIBRARIES)
  endif()

  if(NETTLE_FOUND)
    set(NETTLE_LIBRARIES "-Wl,-Bstatic -lnettle -Wl,-Bdynamic")
    set(HOGWEED_LIBRARIES "-Wl,-Bstatic -lhogweed -Wl,-Bdynamic")
    set(GMP_LIBRARIES "-Wl,-Bstatic -lgmp -Wl,-Bdynamic")
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
  # This ensures that we don't depend on libstdc++ or libgcc_s
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nodefaultlibs")
  set(STATIC_BASE_LIBRARIES "")
  if(ENABLE_ASAN AND NOT WIN32 AND NOT APPLE)
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -Wl,-Bstatic -lasan -Wl,-Bdynamic -ldl -lpthread")
  endif()
  if(ENABLE_TSAN AND NOT WIN32 AND NOT APPLE AND CMAKE_SIZEOF_VOID_P MATCHES 8)
    # libtsan redefines some C++ symbols which then conflict with a
    # statically linked libstdc++. Work around this by allowing multiple
    # definitions. The linker will pick the first one (i.e. the one
    # from libtsan).
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -Wl,-z -Wl,muldefs -Wl,-Bstatic -ltsan -Wl,-Bdynamic -ldl -lm")
  endif()
  if(WIN32)
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -lmingw32 -lgcc_eh -lgcc -lmoldname -lmingwex -lmsvcrt")
    find_package(Threads)
    if(CMAKE_USE_PTHREADS_INIT)
      # pthread has to be statically linked after libraries above and before kernel32
      set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -Wl,-Bstatic -lpthread -Wl,-Bdynamic")
    endif()
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -luser32 -lkernel32 -ladvapi32 -lshell32")
    # mingw has some fun circular dependencies that requires us to link
    # these things again
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -lmingw32 -lgcc_eh -lgcc -lmoldname -lmingwex -lmsvcrt")
  else()
    set(STATIC_BASE_LIBRARIES "${STATIC_BASE_LIBRARIES} -lm -lgcc -lgcc_eh -lc")
  endif()
  set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} ${STATIC_BASE_LIBRARIES}")
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic ${STATIC_BASE_LIBRARIES}")
endif()
