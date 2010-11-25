bin_PROGRAMS = vncconfig

HDRS = Authentication.h Connections.h Desktop.h Hooking.h Inputs.h Legacy.h \
	PasswordDialog.h resource.h Sharing.h

vncconfig_SOURCES = $(HDRS) Legacy.cxx PasswordDialog.cxx vncconfig.cxx

vncconfig_CPPFLAGS = -I$(top_srcdir)/win -I$(top_srcdir)/common

vncconfig_LDADD = $(top_builddir)/win/rfb_win32/librfb_win32.la \
	$(top_builddir)/common/rfb/librfb.la \
	$(top_builddir)/common/Xregion/libXregion.la \
	$(top_builddir)/common/network/libnetwork.la \
	$(top_builddir)/common/rdr/librdr.la -lws2_32 -lgdi32 -lversion -lole32 \
	-lcomctl32 resources.o

EXTRA_DIST = vncconfig.ico vncconfig.rc vncconfig.exe.manifest \
	vncconfig.exe.manifest64 CMakeLists.txt

resources.o: vncconfig.rc
	$(WINDRES) -I$(top_builddir)/win $^ -o $@
