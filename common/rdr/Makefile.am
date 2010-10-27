noinst_LTLIBRARIES = librdr.la

HDRS = Exception.h FdInStream.h FdOutStream.h FixedMemOutStream.h \
	HexInStream.h HexOutStream.h InStream.h MemInStream.h \
	MemOutStream.h msvcwarning.h OutStream.h RandomStream.h \
	SubstitutingInStream.h types.h TLSException.h TLSInStream.h \
	TLSOutStream.h ZlibInStream.h ZlibOutStream.h

librdr_la_SOURCES = $(HDRS) Exception.cxx FdInStream.cxx FdOutStream.cxx \
	HexInStream.cxx HexOutStream.cxx InStream.cxx RandomStream.cxx \
	TLSException.cxx TLSInStream.cxx TLSOutStream.cxx ZlibInStream.cxx \
	ZlibOutStream.cxx

librdr_la_CPPFLAGS = -I$(top_srcdir)/common
librdr_la_LIBADD = @GNUTLS_LIBS@

if INCLUDED_ZLIB
librdr_la_CPPFLAGS += -I$(top_srcdir)/common/zlib
librdr_la_LIBADD += $(top_builddir)/common/zlib/libz.la
endif

EXTRA_DIST = CMakeLists.txt
