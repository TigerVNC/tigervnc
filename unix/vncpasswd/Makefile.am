bin_PROGRAMS = vncpasswd

man1_MANS = vncpasswd.man

vncpasswd_SOURCES = vncpasswd.cxx

vncpasswd_CPPFLAGS = -I$(top_srcdir)/common

vncpasswd_LDADD = $(top_srcdir)/common/rfb/librfb.la \
	$(top_srcdir)/common/rdr/librdr.la

EXTRA_DIST = vncpasswd.man
