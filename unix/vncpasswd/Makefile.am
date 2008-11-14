bin_PROGRAMS = vncpasswd

man1_MANS = vncpasswd.man

vncpasswd_SOURCES = vncpasswd.cxx

vncpasswd_CPPFLAGS = -I$(COMMON_DIR)

vncpasswd_LDADD = $(COMMON_DIR)/rfb/librfb.la $(COMMON_DIR)/rdr/librdr.la

EXTRA_DIST = vncpasswd.man
