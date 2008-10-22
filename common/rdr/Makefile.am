noinst_LTLIBRARIES = librdr.la

HDRS = Exception.h FdInStream.h FdOutStream.h FixedMemOutStream.h \
	HexInStream.h HexOutStream.h InStream.h MemInStream.h \
	MemOutStream.h msvcwarning.h OutStream.h RandomStream.h \
	SubstitutingInStream.h types.h ZlibInStream.h ZlibOutStream.h

librdr_la_SOURCES = $(HDRS) Exception.cxx FdInStream.cxx FdOutStream.cxx \
	InStream.cxx RandomStream.cxx ZlibInStream.cxx ZlibOutStream.cxx \
	HexInStream.cxx HexOutStream.cxx

librdr_la_CPPFLAGS = -I$(top_srcdir)
librdr_la_LIBADD =

if INCLUDED_ZLIB
librdr_la_CPPFLAGS += -I$(top_srcdir)/zlib
librdr_la_LIBADD += $(top_srcdir)/zlib/libz.la
endif
