// This file generates the include file for the assembly
// implementations by abusing the C preprocessor.

;
; Automatically generated include file from jsimdcfg.inc.h
;

#define JPEG_INTERNALS

#include "../jpeglib.h"
#include "../jconfig.h"
#include "../jmorecfg.h"
#include "jsimd.h"

#define define(var) %define _cpp_protection_##var
#define definev(var) %define _cpp_protection_##var var

;
; -- jsimd.h
;

definev(JSIMD_NONE)

; Short forms of external names for systems with brain-damaged linkers.
;
#ifdef NEED_SHORT_EXTERNAL_NAMES
definev(jpeg_simd_cpu_support)
#endif /* NEED_SHORT_EXTERNAL_NAMES */

