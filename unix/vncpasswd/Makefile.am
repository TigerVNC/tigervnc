bin_PROGRAMS = vncpasswd

man1_MANS = vncpasswd.man

vncpasswd_SOURCES = vncpasswd.cxx

vncpasswd_CPPFLAGS = -I$(top_srcdir)/common

vncpasswd_LDADD = $(top_builddir)/common/rfb/librfb.la \
	$(top_builddir)/common/rdr/librdr.la \
	$(top_builddir)/common/os/libos.la

EXTRA_DIST = vncpasswd.man
