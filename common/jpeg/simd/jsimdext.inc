;
; jsimdext.inc - common declarations
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
;
; Based on
; x86 SIMD extension for IJG JPEG library - version 1.02
;
; Copyright (C) 1999-2006, MIYASAKA Masaru.
;
; This software is provided 'as-is', without any express or implied
; warranty.  In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.
;
; [TAB8]

; ==========================================================================
;  System-dependent configurations

%ifdef WIN32	; ----(nasm -fwin32 -DWIN32 ...)--------
; * Microsoft Visual C++
; * MinGW (Minimalist GNU for Windows)
; * CygWin
; * LCC-Win32

; -- segment definition --
;
%define SEG_TEXT    .text  align=16 public use32 class=CODE
%define SEG_CONST   .rdata align=16 public use32 class=CONST

%elifdef OBJ32	; ----(nasm -fobj -DOBJ32 ...)----------
; * Borland C++ (Win32)

; -- segment definition --
;
%define SEG_TEXT    .text  align=16 public use32 class=CODE
%define SEG_CONST   .data  align=16 public use32 class=DATA

%elifdef ELF	; ----(nasm -felf -DELF ...)------------
; * Linux
; * *BSD family Unix using elf format
; * Unix System V, including Solaris x86, UnixWare and SCO Unix

; -- segment definition --
;
%define SEG_TEXT    .text   progbits alloc exec   nowrite align=16
%define SEG_CONST   .rodata progbits alloc noexec nowrite align=16

; To make the code position-independent, append -DPIC to the commandline
;
%define GOT_SYMBOL  _GLOBAL_OFFSET_TABLE_	; ELF supports PIC
%define EXTN(name)  name			; foo() -> foo

%elifdef AOUT	; ----(nasm -faoutb/aout -DAOUT ...)----
; * Older Linux using a.out format  (nasm -f aout -DAOUT ...)
; * *BSD family Unix using a.out format  (nasm -f aoutb -DAOUT ...)

; -- segment definition --
;
%define SEG_TEXT    .text
%define SEG_CONST   .data

; To make the code position-independent, append -DPIC to the commandline
;
%define GOT_SYMBOL  __GLOBAL_OFFSET_TABLE_	; BSD-style a.out supports PIC

%elifdef MACHO	; ----(nasm -fmacho -DMACHO ...)--------
; * NeXTstep/OpenStep/Rhapsody/Darwin/MacOS X (Mach-O format)

; -- segment definition --
;
%define SEG_TEXT    .text  ;align=16	; nasm doesn't accept align=16. why?
%define SEG_CONST   .rodata align=16

; The generation of position-independent code (PIC) is the default on Darwin.
;
%define PIC
%define GOT_SYMBOL  _MACHO_PIC_		; Mach-O style code-relative addressing

%else		; ----(Other case)----------------------

; -- segment definition --
;
%define SEG_TEXT    .text
%define SEG_CONST   .data

%endif	; ----------------------------------------------

; ==========================================================================

; --------------------------------------------------------------------------
;  Common types
;
%define POINTER                 dword           ; general pointer type
%define SIZEOF_POINTER          SIZEOF_DWORD    ; sizeof(POINTER)
%define POINTER_BIT             DWORD_BIT       ; sizeof(POINTER)*BYTE_BIT

; --------------------------------------------------------------------------
;  External Symbol Name
;
%ifndef EXTN
%define EXTN(name)   _ %+ name		; foo() -> _foo
%endif

; --------------------------------------------------------------------------
;  Macros for position-independent code (PIC) support
;
%ifndef GOT_SYMBOL
%undef PIC
%endif

%ifdef PIC ; -------------------------------------------

%ifidn GOT_SYMBOL,_MACHO_PIC_ ; --------------------

; At present, nasm doesn't seem to support PIC generation for Mach-O.
; The PIC support code below is a little tricky.

	SECTION	SEG_CONST
const_base:

%define GOTOFF(got,sym) (got) + (sym) - const_base

%imacro get_GOT	1
	; NOTE: this macro destroys ecx resister.
	call	%%geteip
	add	ecx, byte (%%ref - $)
	jmp	short %%adjust
%%geteip:
	mov	ecx, POINTER [esp]
	ret
%%adjust:
	push	ebp
	xor	ebp,ebp		; ebp = 0
%ifidni %1,ebx	; (%1 == ebx)
	; db 0x8D,0x9C + jmp near const_base =
	;   lea ebx, [ecx+ebp*8+(const_base-%%ref)] ; 8D,9C,E9,(offset32)
	db	0x8D,0x9C		; 8D,9C
	jmp	near const_base		; E9,(const_base-%%ref)
%%ref:
%else  ; (%1 != ebx)
	; db 0x8D,0x8C + jmp near const_base =
	;   lea ecx, [ecx+ebp*8+(const_base-%%ref)] ; 8D,8C,E9,(offset32)
	db	0x8D,0x8C		; 8D,8C
	jmp	near const_base		; E9,(const_base-%%ref)
%%ref:	mov	%1, ecx
%endif ; (%1 == ebx)
	pop	ebp
%endmacro

%else	; GOT_SYMBOL != _MACHO_PIC_ ----------------

%define GOTOFF(got,sym) (got) + (sym) wrt ..gotoff

%imacro get_GOT	1
	extern	GOT_SYMBOL
	call	%%geteip
	add	%1, GOT_SYMBOL + $$ - $ wrt ..gotpc
	jmp	short %%done
%%geteip:
	mov	%1, POINTER [esp]
	ret
%%done:
%endmacro

%endif	; GOT_SYMBOL == _MACHO_PIC_ ----------------

%imacro pushpic	1.nolist
	push	%1
%endmacro
%imacro poppic	1.nolist
	pop	%1
%endmacro
%imacro movpic	2.nolist
	mov	%1,%2
%endmacro

%else	; !PIC -----------------------------------------

%define GOTOFF(got,sym) (sym)

%imacro get_GOT	1.nolist
%endmacro
%imacro pushpic	1.nolist
%endmacro
%imacro poppic	1.nolist
%endmacro
%imacro movpic	2.nolist
%endmacro

%endif	;  PIC -----------------------------------------

; --------------------------------------------------------------------------
;  Align the next instruction on {2,4,8,16,..}-byte boundary.
;  ".balign n,,m" in GNU as
;
%define MSKLE(x,y)  (~(((y) & 0xFFFF) - ((x) & 0xFFFF)) >> 16)
%define FILLB(b,n)  (($$-(b)) & ((n)-1))

%imacro alignx 1-2.nolist 0xFFFF
%%bs:	times MSKLE(FILLB(%%bs,%1),%2) & MSKLE(16,FILLB($,%1)) & FILLB($,%1) \
	       db 0x90                               ; nop
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/9 \
	       db 0x8D,0x9C,0x23,0x00,0x00,0x00,0x00 ; lea ebx,[ebx+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/7 \
	       db 0x8D,0xAC,0x25,0x00,0x00,0x00,0x00 ; lea ebp,[ebp+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/6 \
	       db 0x8D,0xAD,0x00,0x00,0x00,0x00      ; lea ebp,[ebp+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/4 \
	       db 0x8D,0x6C,0x25,0x00                ; lea ebp,[ebp+0x00]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/3 \
	       db 0x8D,0x6D,0x00                     ; lea ebp,[ebp+0x00]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/2 \
	       db 0x8B,0xED                          ; mov ebp,ebp
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/1 \
	       db 0x90                               ; nop
%endmacro

; Align the next data on {2,4,8,16,..}-byte boundary.
;
%imacro alignz 1.nolist
	align %1, db 0		; filling zeros
%endmacro


; --------------------------------------------------------------------------
;  Defines picked up from the C headers
;
%include "simd/jsimdcfg.inc"

; --------------------------------------------------------------------------
