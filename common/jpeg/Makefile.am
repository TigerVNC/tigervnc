noinst_LTLIBRARIES = libjpeg.la
noinst_HEADERS = jerror.h jmorecfg.h jpeglib.h
nodist_noinst_HEADERS = jconfig.h

HDRS = jchuff.h jdct.h jdhuff.h jerror.h jinclude.h jmemsys.h jmorecfg.h \
	jpegint.h jpeglib.h jversion.h jsimd.h jsimddct.h

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

noinst_PROGRAMS = jpgtest jpegut cjpeg djpeg jpegtran

jpgtest_SOURCES = $(TSTHDRS) jpgtest.cxx bmp.h bmp.c turbojpegl.c

jpgtest_LDADD = libjpeg.la

jpegut_SOURCES = $(TSTHDRS) jpegut.c bmp.h bmp.c turbojpegl.c

jpegut_LDADD = libjpeg.la

cjpeg_SOURCES = cdjpeg.h cderror.h cdjpeg.c cjpeg.c rdbmp.c rdgif.c \
	rdppm.c rdswitch.c rdtarga.c 

cjpeg_LDADD = libjpeg.la

cjpeg_CFLAGS = -DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED \
	-DTARGA_SUPPORTED

djpeg_SOURCES = cdjpeg.h cderror.h cdjpeg.c djpeg.c rdcolmap.c rdswitch.c \
	wrbmp.c wrgif.c wrppm.c wrtarga.c

djpeg_LDADD = libjpeg.la

djpeg_CFLAGS = -DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED \
	-DTARGA_SUPPORTED

jpegtran_SOURCES = jpegtran.c rdswitch.c cdjpeg.c transupp.c transupp.h

jpegtran_LDADD = libjpeg.la


DOCS= jconfig.doc README README-turbo.txt LICENSE.txt LGPL.txt \
	README_TigerVNC.txt

TESTFILES= testorig.jpg testorig.ppm testimg.bmp testimgflt.jpg \
	testimgfst.jpg testimgint.jpg testimgp.jpg testimgflt.ppm testimgfst.ppm \
	testimgint.ppm testimgflt-nosimd.jpg

EXTRA_DIST = win $(DOCS) $(TESTFILES) CMakeLists.txt

dist-hook:
	rm -rf `find $(distdir) -name .svn`


if WITH_SIMD

test: testclean all
	./jpegut
	./cjpeg -dct int -outfile testoutint.jpg $(srcdir)/testorig.ppm
	./cjpeg -dct fast -opt -outfile testoutfst.jpg $(srcdir)/testorig.ppm
	./cjpeg -dct float -outfile testoutflt.jpg $(srcdir)/testorig.ppm
	cmp $(srcdir)/testimgint.jpg testoutint.jpg
	cmp $(srcdir)/testimgfst.jpg testoutfst.jpg
	cmp $(srcdir)/testimgflt.jpg testoutflt.jpg
	./djpeg -dct int -fast -ppm -outfile testoutint.ppm $(srcdir)/testorig.jpg
	./djpeg -dct fast -ppm -outfile testoutfst.ppm $(srcdir)/testorig.jpg
	./djpeg -dct float -ppm -outfile testoutflt.ppm $(srcdir)/testorig.jpg
	cmp $(srcdir)/testimgint.ppm testoutint.ppm
	cmp $(srcdir)/testimgfst.ppm testoutfst.ppm
	cmp $(srcdir)/testimgflt.ppm testoutflt.ppm
	./djpeg -dct int -bmp -colors 256 -outfile testout.bmp  $(srcdir)/testorig.jpg
	cmp $(srcdir)/testimg.bmp testout.bmp
	./cjpeg -dct int -progressive -outfile testoutp.jpg $(srcdir)/testorig.ppm
	cmp $(srcdir)/testimgp.jpg testoutp.jpg
	./jpegtran -outfile testoutt.jpg testoutp.jpg
	cmp $(srcdir)/testimgint.jpg testoutt.jpg

else

test: testclean all
	./jpegut
	./cjpeg -dct int -outfile testoutint.jpg $(srcdir)/testorig.ppm
	./cjpeg -dct fast -opt -outfile testoutfst.jpg $(srcdir)/testorig.ppm
	./cjpeg -dct float -outfile testoutflt.jpg $(srcdir)/testorig.ppm
	cmp $(srcdir)/testimgint.jpg testoutint.jpg
	cmp $(srcdir)/testimgfst.jpg testoutfst.jpg
	cmp $(srcdir)/testimgflt-nosimd.jpg testoutflt.jpg
	./djpeg -dct int -fast -ppm -outfile testoutint.ppm $(srcdir)/testorig.jpg
	./djpeg -dct fast -ppm -outfile testoutfst.ppm $(srcdir)/testorig.jpg
	./djpeg -dct float -ppm -outfile testoutflt.ppm $(srcdir)/testorig.jpg
	cmp $(srcdir)/testimgint.ppm testoutint.ppm
	cmp $(srcdir)/testimgfst.ppm testoutfst.ppm
	cmp $(srcdir)/testorig.ppm testoutflt.ppm
	./djpeg -dct int -bmp -colors 256 -outfile testout.bmp  $(srcdir)/testorig.jpg
	cmp $(srcdir)/testimg.bmp testout.bmp
	./cjpeg -dct int -progressive -outfile testoutp.jpg $(srcdir)/testorig.ppm
	cmp $(srcdir)/testimgp.jpg testoutp.jpg
	./jpegtran -outfile testoutt.jpg testoutp.jpg
	cmp $(srcdir)/testimgint.jpg testoutt.jpg

endif

testclean:
	rm -f testout*
	rm -f *_GRAYQ[0-9]*.bmp
	rm -f *_GRAYQ[0-9]*.ppm
	rm -f *_GRAYQ[0-9]*.jpg
	rm -f *_420Q[0-9]*.bmp
	rm -f *_420Q[0-9]*.ppm
	rm -f *_420Q[0-9]*.jpg
	rm -f *_422Q[0-9]*.bmp
	rm -f *_422Q[0-9]*.ppm
	rm -f *_422Q[0-9]*.jpg
	rm -f *_444Q[0-9]*.bmp
	rm -f *_444Q[0-9]*.ppm
	rm -f *_444Q[0-9]*.jpg
