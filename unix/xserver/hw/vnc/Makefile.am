TIGERVNC_SRCDIR=${top_srcdir}/../..
LIB_DIR=${top_builddir}/../../common

RFB_LIB=$(LIB_DIR)/rfb/librfb.la
RDR_LIB=$(LIB_DIR)/rdr/librdr.la
NETWORK_LIB=$(LIB_DIR)/network/libnetwork.la
XREGION_LIB=$(LIB_DIR)/Xregion/libXregion.la
COMMON_LIBS=$(NETWORK_LIB) $(RFB_LIB) $(RDR_LIB) $(XREGION_LIB)

noinst_LTLIBRARIES = libvnccommon.la

HDRS = RegionHelper.h vncExtInit.h vncHooks.h XserverDesktop.h xorg-version.h \
	Input.h

libvnccommon_la_SOURCES = $(HDRS) vncExtInit.cc vncHooks.cc XserverDesktop.cc \
	Input.cc

libvnccommon_la_CPPFLAGS = -DVENDOR_RELEASE="$(VENDOR_RELEASE)" \
	-DVENDOR_STRING="\"$(VENDOR_STRING)\"" -I$(TIGERVNC_SRCDIR)/common -UHAVE_CONFIG_H \
	-I$(TIGERVNC_SRCDIR)/unix/vncconfig $(XVNC_CPPFLAGS) ${XSERVERLIBS_CFLAGS} -I$(includedir) \
	-I$(top_srcdir)/include

bin_PROGRAMS = Xvnc

man1_MANS = Xvnc.man

Xvnc_SOURCES = xvnc.cc \
	$(top_srcdir)/Xi/stubs.c $(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/fb/fbcmap_mi.c buildtime.c

nodist_Xvnc_SOURCES = fbrop.h fb.h pixman.h

Xvnc_CPPFLAGS = $(XVNC_CPPFLAGS) -DTIGERVNC -DNO_MODULE_EXTS \
	-UHAVE_CONFIG_H \
	-DXFree86Server -DVENDOR_RELEASE="$(VENDOR_RELEASE)" \
	-DVENDOR_STRING="\"$(VENDOR_STRING)\"" -I$(TIGERVNC_SRCDIR)/common \
	-I$(top_srcdir)/include ${XSERVERLIBS_CFLAGS} -I$(includedir)

Xvnc_LDADD = $(XVNC_LIBS) libvnccommon.la $(COMMON_LIBS) \
	$(XSERVER_LIBS) $(XSERVER_SYS_LIBS) $(XVNC_SYS_LIBS) -lX11

Xvnc_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)

libvnc_la_LTLIBRARIES = libvnc.la
libvnc_ladir = $(moduledir)/extensions

libvnc_la_SOURCES = xf86vncModule.cc

libvnc_la_CPPFLAGS = $(XVNC_CPPFLAGS) -I$(TIGERVNC_SRCDIR)/common -UHAVE_CONFIG_H \
	-I$(top_srcdir)/hw/xfree86/common \
	-I$(top_srcdir)/hw/xfree86/os-support \
	-I$(top_srcdir)/hw/xfree86/os-support/bus \
	-I$(top_srcdir)/include \
	${XSERVERLIBS_CFLAGS} -I$(includedir)

libvnc_la_LDFLAGS = -module -avoid-version

libvnc_la_LIBADD = libvnccommon.la $(COMMON_LIBS)

EXTRA_DIST = Xvnc.man

# C++ hacks
BUILT_SOURCES = $(nodist_Xvnc_SOURCES)

fb.h: $(top_srcdir)/fb/fb.h
	cat $(top_srcdir)/fb/fb.h | sed -e 's,and,c_and,' -e 's,xor,c_xor,' > $(srcdir)/fb.h

pixman.h:
	for i in ${XSERVERLIBS_CFLAGS}; do \
		if [[ "$$i" =~ "pixman" ]]; then \
			PIXMANINCDIR=`echo $$i | sed s/-I//g`; \
		fi; \
	done; \
	if [ ! "$$PIXMANINCDIR" = "" ]; then \
		cat $$PIXMANINCDIR/pixman.h | sed 's/xor/c_xor/' > $(srcdir)/pixman.h; \
	else \
		echo Pixman include directory not set in XSERVERLIBS_CFLAGS \(perhaps Pixman was not found by configure?\); \
	fi

fbrop.h: $(top_srcdir)/fb/fbrop.h
	cat $(top_srcdir)/fb/fbrop.h | sed -e 's,and,c_and,' -e 's,xor,c_xor,' > $(srcdir)/fbrop.h
