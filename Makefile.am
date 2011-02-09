SUBDIRS = common po

if BUILD_WIN
SUBDIRS += win
else
SUBDIRS += unix
endif

ACLOCAL_AMFLAGS = -I m4

if X86_64

udmg: all
	sh makemacpkg universal ${BUILDDIR32}

endif

dmg: all
	sh makemacpkg

EXTRA_DIST = CMakeLists.txt cmakescripts/getdate.bat LICENCE.TXT \
	doc java media release config.h.cmake.in

dist-hook:
	rm -rf `find $(distdir) -name .svn`
