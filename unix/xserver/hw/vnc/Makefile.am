TIGERVNC_SRCDIR=${top_srcdir}/../..
TIGERVNC_BUILDDIR=${TIGERVNC_SRCDIR}

RFB_LIB=$(TIGERVNC_BUILDDIR)/common/rfb/librfb.la
RDR_LIB=$(TIGERVNC_BUILDDIR)/common/rdr/librdr.la
OS_LIB=$(TIGERVNC_BUILDDIR)/common/os/libos.la
NETWORK_LIB=$(TIGERVNC_BUILDDIR)/common/network/libnetwork.la
UNIXCOMMON_LIB=$(TIGERVNC_BUILDDIR)/unix/common/libunixcommon.la
COMMON_LIBS=$(NETWORK_LIB) $(RFB_LIB) $(RDR_LIB) $(OS_LIB) $(UNIXCOMMON_LIB)

noinst_LTLIBRARIES = libvnccommon.la

HDRS = vncExtInit.h vncHooks.h \
	vncBlockHandler.h vncSelection.h \
	XorgGlue.h XserverDesktop.h xorg-version.h \
	vncInput.h RFBGlue.h

libvnccommon_la_SOURCES = $(HDRS) \
	vncExt.c vncExtInit.cc vncHooks.c vncSelection.c \
	vncBlockHandler.c XorgGlue.c RandrGlue.c RFBGlue.cc XserverDesktop.cc \
	vncInput.c vncInputXKB.c qnum_to_xorgevdev.c qnum_to_xorgkbd.c

libvnccommon_la_CPPFLAGS = -DVENDOR_RELEASE="$(VENDOR_RELEASE)" -I$(TIGERVNC_SRCDIR)/unix/common \
	-DVENDOR_STRING="\"$(VENDOR_STRING)\"" -I$(TIGERVNC_SRCDIR)/common -UHAVE_CONFIG_H \
	-I$(TIGERVNC_SRCDIR)/unix/vncconfig $(XVNC_CPPFLAGS) ${XSERVERLIBS_CFLAGS} \
	-I$(top_srcdir)/include

bin_PROGRAMS = Xvnc

man1_MANS = Xvnc.man

Xvnc_SOURCES = xvnc.c \
	$(top_srcdir)/Xi/stubs.c $(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/fb/fbcmap_mi.c buildtime.c

# Xvnc contains no C++ sources so automake doesn't understand that we
# need to use the C++ compiler to link things. This is the upstream
# recommendation for coaxing automake.
nodist_EXTRA_Xvnc_SOURCES = dummy.cxx

Xvnc_CPPFLAGS = $(XVNC_CPPFLAGS) -DTIGERVNC -DNO_MODULE_EXTS \
	-UHAVE_CONFIG_H \
	-DXFree86Server -DVENDOR_RELEASE="$(VENDOR_RELEASE)" \
	-DVENDOR_STRING="\"$(VENDOR_STRING)\"" -I$(TIGERVNC_SRCDIR)/common -I$(TIGERVNC_SRCDIR)/unix/common \
	-I$(top_srcdir)/include ${XSERVERLIBS_CFLAGS}

LOCAL_LIBS = \
	$(XVNC_LIBS) \
	$(XSERVER_LIBS) \
	libvnccommon.la \
	$(COMMON_LIBS)

Xvnc_DEPENDENCIES = $(LOCAL_LIBS)
Xvnc_LDADD = $(LOCAL_LIBS) $(XSERVER_SYS_LIBS) $(XVNC_SYS_LIBS) -lX11

Xvnc_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)

libvnc_la_LTLIBRARIES = libvnc.la
libvnc_ladir = $(moduledir)/extensions

libvnc_la_SOURCES = vncModule.c

# See Xvnc magic above
nodist_EXTRA_libvnc_la_SOURCES = dummy.cxx

libvnc_la_CPPFLAGS = $(XVNC_CPPFLAGS) -I$(TIGERVNC_SRCDIR)/common -UHAVE_CONFIG_H \
	-I$(TIGERVNC_SRCDIR)/unix/common \
	-I$(top_srcdir)/hw/xfree86/common \
	-I$(top_srcdir)/hw/xfree86/os-support \
	-I$(top_srcdir)/hw/xfree86/os-support/bus \
	-I$(top_srcdir)/include \
	${XSERVERLIBS_CFLAGS}

libvnc_la_LDFLAGS = -module -avoid-version -Wl,-z,now

libvnc_la_LIBADD = libvnccommon.la $(COMMON_LIBS)

EXTRA_DIST = Xvnc.man
