;
; jsimdcpu.asm - SIMD instruction support check
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright 2009 D. R. Commander
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

%include "jsimdext.inc"

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	64
;
; Check if the CPU supports SIMD instructions
;
; GLOBAL(unsigned int)
; jpeg_simd_cpu_support (void)
;

	align	16
	global	EXTN(jpeg_simd_cpu_support)

EXTN(jpeg_simd_cpu_support):
	push	rbx

	xor	rdi,rdi			; simd support flag

	pushfq
	pop	rax
	mov	rdx,rax
	xor	rax, 1<<21		; flip ID bit in EFLAGS
	push	rax
	popfq
	pushfq
	pop	rax
	xor	rax,rdx
	jz	short .return		; CPUID is not supported

	; Check for MMX instruction support
	xor	rax,rax
	cpuid
	test	rax,rax
	jz	short .return

	xor	rax,rax
	inc	rax
	cpuid
	mov	rax,rdx			; rax = Standard feature flags

	test	rax, 1<<23		; bit23:MMX
	jz	short .no_mmx
	or	rdi, byte JSIMD_MMX
.no_mmx:
	test	rax, 1<<25		; bit25:SSE
	jz	short .no_sse
	or	rdi, byte JSIMD_SSE
.no_sse:
	test	rax, 1<<26		; bit26:SSE2
	jz	short .no_sse2
	or	rdi, byte JSIMD_SSE2
.no_sse2:

	; Check for 3DNow! instruction support
	mov	eax, 0x80000000
	cpuid
	cmp	eax, 0x80000000
	jbe	short .return

	mov	rax, 0x80000001
	cpuid
	mov	rax,rdx			; eax = Extended feature flags

	test	eax, 1<<31		; bit31:3DNow!(vendor independent)
	jz	short .no_3dnow
	or	edi, byte JSIMD_3DNOW
.no_3dnow:

.return:
	mov	rax,rdi

	pop	rbx
	ret

