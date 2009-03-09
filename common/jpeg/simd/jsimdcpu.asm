;
; jsimdcpu.asm - SIMD instruction support check
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
;
; Based on
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; This file should be assembled with NASM (Netwide Assembler),
; can *not* be assembled with Microsoft's MASM or any compatible
; assembler (including Borland's Turbo Assembler).
; NASM is available from http://nasm.sourceforge.net/ or
; http://sourceforge.net/project/showfiles.php?group_id=6208
;
; [TAB8]

%include "simd/jsimdext.inc"

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Check if the CPU supports SIMD instructions
;
; GLOBAL(unsigned int)
; jpeg_simd_cpu_support (void)
;

	align	16
	global	EXTN(jpeg_simd_cpu_support)

EXTN(jpeg_simd_cpu_support):
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
;	push	esi		; unused
	push	edi

	xor	edi,edi			; simd support flag

	pushfd
	pop	eax
	mov	edx,eax
	xor	eax, 1<<21		; flip ID bit in EFLAGS
	push	eax
	popfd
	pushfd
	pop	eax
	xor	eax,edx
	jz	short .return		; CPUID is not supported

	; Check for MMX instruction support
	xor	eax,eax
	cpuid
	test	eax,eax
	jz	short .return

	xor	eax,eax
	inc	eax
	cpuid
	mov	eax,edx			; eax = Standard feature flags

	test	eax, 1<<23		; bit23:MMX
	jz	short .no_mmx
	or	edi, byte JSIMD_MMX
.no_mmx:

.return:
	mov	eax,edi

	pop	edi
;	pop	esi		; unused
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	ret

