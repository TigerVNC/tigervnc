noinst_LTLIBRARIES = libos.la

HDRS = net.h print.h os.h tls.h

libos_la_SOURCES = $(HDRS) print.c net.c os.cxx tls.cxx

libos_la_CPPFLAGS = -I$(top_srcdir)/common

EXTRA_DIST = CMakeLists.txt
