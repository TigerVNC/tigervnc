/*
 * simd/jsimd.h
 *
 * Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * Based on the x86 SIMD extension for IJG JPEG library,
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 *
 */

/* Bitmask for supported acceleration methods */

#define JSIMD_NONE    0x00

/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_simd_cpu_support	jSiCpuSupport
#endif /* NEED_SHORT_EXTERNAL_NAMES */

/* SIMD Ext: retrieve SIMD/CPU information */
EXTERN(unsigned int) jpeg_simd_cpu_support JPP((void));

