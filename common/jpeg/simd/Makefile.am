noinst_LTLIBRARIES = libsimd.la

BUILT_SOURCES = jsimdcfg.inc

EXTRA_DIST = nasm_lt.sh

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

.asm.lo:
	$(LIBTOOL) --mode=compile --tag NASM ./nasm_lt.sh $(NASM) $(NAFLAGS) $< -o $@

jsimdcfg.inc: jsimdcfg.inc.h ../jpeglib.h ../jconfig.h ../jmorecfg.h
	$(CPP) $< | grep ^[\;%] | sed 's%_cpp_protection_%%' > $@

