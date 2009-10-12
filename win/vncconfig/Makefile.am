bin_PROGRAMS = vncconfig

HDRS = Authentication.h Connections.h Desktop.h Hooking.h Inputs.h Legacy.h \
	PasswordDialog.h resource.h Sharing.h

vncconfig_SOURCES = $(HDRS) Legacy.cxx PasswordDialog.cxx vncconfig.cxx

vncconfig_CPPFLAGS = -I$(top_srcdir)/win -I$(top_srcdir)/common

vncconfig_LDADD = $(top_srcdir)/win/rfb_win32/librfb_win32.la \
	$(top_srcdir)/common/rfb/librfb.la \
	$(top_srcdir)/common/Xregion/libXregion.la \
	$(top_srcdir)/common/network/libnetwork.la \
	$(top_srcdir)/common/rdr/librdr.la -lws2_32 -lgdi32 -lversion -lole32 \
	-lcomctl32 resources.o

EXTRA_DIST = connected.ico vncconfig.ico vncconfig.rc vncconfig.exe.manifest

resources.o: vncconfig.rc
	$(WINDRES) $^ -o $@
