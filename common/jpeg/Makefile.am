noinst_LTLIBRARIES = libjpeg.la

HDRS = jchuff.h jdct.h jdhuff.h jerror.h jinclude.h jmemsys.h jmorecfg.h \
	jpegint.h jpeglib.h jversion.h jsimd.h jsimddct.h

libjpeg_la_SOURCES = $(HDRS) jcapimin.c jcapistd.c jccoefct.c jccolor.c \
	jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c \
	jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c jctrans.c \
	jdapimin.c jdapistd.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c \
	jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c \
	jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c jerror.c \
	jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c \
	jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c \
	jsimd.c

if WITH_SIMD

BUILT_SOURCES = simd/jsimdcfg.inc

EXTRA_DIST = nasm_lt.sh

libjpeg_la_SOURCES += simd/jsimd.h simd/jsimdcfg.inc.h \
	simd/jsimdext.inc simd/jcolsamp.inc simd/jdct.inc \
	simd/jsimdcpu.asm \
	simd/jccolmmx.asm simd/jdcolmmx.asm \
	simd/jcsammmx.asm simd/jdsammmx.asm simd/jdmermmx.asm \
	simd/jcqntmmx.asm simd/jfmmxfst.asm simd/jfmmxint.asm \
	simd/jimmxred.asm simd/jimmxint.asm simd/jimmxfst.asm

endif

.asm.lo:
	$(LIBTOOL) --mode=compile --tag NASM ./nasm_lt.sh $(NASM) $(NAFLAGS) $< -o $@

simd/jsimdcfg.inc: simd/jsimdcfg.inc.h jpeglib.h jconfig.h jmorecfg.h
	$(CPP) $< | grep ^[\;%] | sed 's%_cpp_protection_%%' > $@

