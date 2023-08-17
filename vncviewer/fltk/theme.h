/* Copyright 2023 Pierre Ossman for Cendio AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __FLTK_THEME_H__
#define __FLTK_THEME_H__

#include <FL/Enumerations.H>

#define _THEME_BOX_BASE         (FL_FREE_BOXTYPE+20)

#define THEME_UP_FRAME          (Fl_Boxtype)(_THEME_BOX_BASE+0)
#define THEME_DOWN_FRAME        (Fl_Boxtype)(_THEME_BOX_BASE+1)
#define THEME_THIN_UP_FRAME     (Fl_Boxtype)(_THEME_BOX_BASE+2)
#define THEME_THIN_DOWN_FRAME   (Fl_Boxtype)(_THEME_BOX_BASE+3)

#define THEME_UP_BOX            (Fl_Boxtype)(_THEME_BOX_BASE+4)
#define THEME_DOWN_BOX          (Fl_Boxtype)(_THEME_BOX_BASE+5)
#define THEME_THIN_UP_BOX       (Fl_Boxtype)(_THEME_BOX_BASE+6)
#define THEME_THIN_DOWN_BOX     (Fl_Boxtype)(_THEME_BOX_BASE+7)
#define THEME_ROUND_UP_BOX      (Fl_Boxtype)(_THEME_BOX_BASE+8)
#define THEME_ROUND_DOWN_BOX    (Fl_Boxtype)(_THEME_BOX_BASE+9)

void init_theme();

#endif
