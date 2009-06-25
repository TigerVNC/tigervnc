noinst_LTLIBRARIES = libsimd.la

BUILT_SOURCES = jsimdcfg.inc

EXTRA_DIST = nasm_lt.sh

if WITH_SIMD64

libsimd_la_SOURCES = jsimd.h jsimdcfg.inc.h \
	jsimdext.inc jcolsamp.inc jdct.inc \
	jsimdcpu-64.asm jfsseflt-64.asm \
	jccolss2-64.asm jdcolss2-64.asm \
	jcsamss2-64.asm jdsamss2-64.asm jdmerss2-64.asm \
	jcqnts2i-64.asm jfss2fst-64.asm jfss2int-64.asm \
	jiss2red-64.asm jiss2int-64.asm jiss2fst-64.asm \
	jcqnts2f-64.asm jiss2flt-64.asm

jccolss2-64.lo: jcclrss2-64.asm
jdcolss2-64.lo: jdclrss2-64.asm
jdmerss2-64.lo: jdmrgss2-64.asm

else

libsimd_la_SOURCES = jsimd.h jsimdcfg.inc.h \
	jsimdext.inc jcolsamp.inc jdct.inc \
	jsimdcpu.asm \
	jccolmmx.asm jdcolmmx.asm \
	jcsammmx.asm jdsammmx.asm jdmermmx.asm \
	jcqntmmx.asm jfmmxfst.asm jfmmxint.asm \
	jimmxred.asm jimmxint.asm jimmxfst.asm \
	jcqnt3dn.asm jf3dnflt.asm ji3dnflt.asm \
	jcqntsse.asm jfsseflt.asm jisseflt.asm \
	jccolss2.asm jdcolss2.asm \
	jcsamss2.asm jdsamss2.asm jdmerss2.asm \
	jcqnts2i.asm jfss2fst.asm jfss2int.asm \
	jiss2red.asm jiss2int.asm jiss2fst.asm \
	jcqnts2f.asm jiss2flt.asm

jccolmmx.lo: jcclrmmx.asm
jccolss2.lo: jcclrss2.asm
jdcolmmx.lo: jdclrmmx.asm
jdcolss2.lo: jdclrss2.asm
jdmermmx.lo: jdmrgmmx.asm
jdmerss2.lo: jdmrgss2.asm

endif

.asm.lo:
	$(LIBTOOL) --mode=compile --tag NASM ./nasm_lt.sh $(NASM) $(NAFLAGS) $< -o $@

jsimdcfg.inc: jsimdcfg.inc.h ../jpeglib.h ../jconfig.h ../jmorecfg.h
	$(CPP) $< | grep ^[\;%] | sed 's%_cpp_protection_%%' > $@

