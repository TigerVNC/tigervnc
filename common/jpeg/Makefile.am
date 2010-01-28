if WITH_SHARED

lib_LTLIBRARIES = libjpeg.la
libjpeg_la_LDFLAGS = -version-number 62:0:0 -no-undefined
include_HEADERS = jconfig.h jerror.h jmorecfg.h jpeglib.h

else

noinst_LTLIBRARIES = libjpeg.la
noinst_HEADERS = jconfig.h jerror.h jmorecfg.h jpeglib.h

endif

HDRS = jchuff.h jdct.h jdhuff.h jerror.h jinclude.h jmemsys.h jmorecfg.h \
	jpegint.h jpeglib.h jversion.h jsimd.h jsimddct.h turbojpeg.h

libjpeg_la_SOURCES = $(HDRS) jcapimin.c jcapistd.c jccoefct.c jccolor.c \
	jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c \
	jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c jctrans.c \
	jdapimin.c jdapistd.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c \
	jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c \
	jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c jerror.c \
	jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c \
	jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c

if WITH_SIMD

SUBDIRS = simd
libjpeg_la_LIBADD = simd/libsimd.la

else

libjpeg_la_SOURCES += jsimd_none.c

endif

TSTHDRS = turbojpeg.h rrutil.h rrtimer.h

noinst_PROGRAMS = jpgtest jpegut cjpeg djpeg

jpgtest_SOURCES = $(TSTHDRS) jpgtest.cxx bmp.h bmp.c turbojpegl.c

jpgtest_LDADD = $(top_srcdir)/libjpeg.la

jpegut_SOURCES = $(TSTHDRS) jpegut.c bmp.h bmp.c turbojpegl.c

jpegut_LDADD = $(top_srcdir)/libjpeg.la

cjpeg_SOURCES = cdjpeg.h cderror.h cdjpeg.c cjpeg.c rdbmp.c rdgif.c \
	rdppm.c rdswitch.c rdtarga.c 

cjpeg_LDADD = $(top_srcdir)/libjpeg.la

cjpeg_CFLAGS = -DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED \
	-DTARGA_SUPPORTED

djpeg_SOURCES = cdjpeg.h cderror.h cdjpeg.c djpeg.c rdcolmap.c rdswitch.c \
	wrbmp.c wrgif.c wrppm.c wrtarga.c

djpeg_LDADD = $(top_srcdir)/libjpeg.la

djpeg_CFLAGS = -DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED \
	-DTARGA_SUPPORTED
