SUBDIRS = rfb_win32 vncviewer

if BUILD_WINVNC
SUBDIRS += vncconfig winvnc
endif

EXTRA_DIST = logmessages LICENCE.TXT README.txt README_BINARY.txt \
	resdefs.h.in tigervnc.iss.in wm_hooks CMakeLists.txt resdefs.h.cmake.in

dist-hook:
	rm -rf `find $(distdir) -name .svn`
