;
; jcolsamp.inc - private declarations for color conversion & up/downsampling
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
;
; Based on
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; [TAB8]

; --------------------------------------------------------------------------

; pseudo-resisters to make ordering of RGB configurable
;
%if RGB_RED == 0
%define  mmA  mm0
%define  mmB  mm1
%elif RGB_GREEN == 0
%define  mmA  mm2
%define  mmB  mm3
%elif RGB_BLUE == 0
%define  mmA  mm4
%define  mmB  mm5
%else
%define  mmA  mm6
%define  mmB  mm7
%endif

%if RGB_RED == 1
%define  mmC  mm0
%define  mmD  mm1
%elif RGB_GREEN == 1
%define  mmC  mm2
%define  mmD  mm3
%elif RGB_BLUE == 1
%define  mmC  mm4
%define  mmD  mm5
%else
%define  mmC  mm6
%define  mmD  mm7
%endif

%if RGB_RED == 2
%define  mmE  mm0
%define  mmF  mm1
%elif RGB_GREEN == 2
%define  mmE  mm2
%define  mmF  mm3
%elif RGB_BLUE == 2
%define  mmE  mm4
%define  mmF  mm5
%else
%define  mmE  mm6
%define  mmF  mm7
%endif

%if RGB_RED == 3
%define  mmG  mm0
%define  mmH  mm1
%elif RGB_GREEN == 3
%define  mmG  mm2
%define  mmH  mm3
%elif RGB_BLUE == 3
%define  mmG  mm4
%define  mmH  mm5
%else
%define  mmG  mm6
%define  mmH  mm7
%endif

; --------------------------------------------------------------------------
