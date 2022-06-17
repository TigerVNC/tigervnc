/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __FLTK_LAYOUT_H__
#define __FLTK_LAYOUT_H__

#include <FL/fl_draw.H>

/* Calculates the width of a string as printed by FLTK (pixels) */
static inline int gui_str_len(const char *str)
{
    float len;

    fl_font(FL_HELVETICA, FL_NORMAL_SIZE);

    len = fl_width(str);

    return (int)(len + 0.5f);
}

/**** MARGINS ****/

#define OUTER_MARGIN            15
#define INNER_MARGIN            10

/* Tighter grouping of related fields */
#define TIGHT_MARGIN            5

/**** ADJUSTMENTS ****/
#define INDENT                  10

/**** FLTK WIDGETS ****/

/* Fl_Input */
#define INPUT_LABEL_OFFSET      FL_NORMAL_SIZE
#define INPUT_HEIGHT            25

/* Fl_Button */
#define BUTTON_WIDTH            115
#define BUTTON_HEIGHT           27

/* Fl_Round_Button */
#define RADIO_MIN_WIDTH         (FL_NORMAL_SIZE + 5)
#define RADIO_HEIGHT            (FL_NORMAL_SIZE + 7)

/* Fl_Check_Button */
#define CHECK_MIN_WIDTH         RADIO_MIN_WIDTH
#define CHECK_HEIGHT            RADIO_HEIGHT

/* Fl_Choice */

#define CHOICE_HEIGHT           INPUT_HEIGHT

/* Fl_Group */
#define GROUP_LABEL_OFFSET      FL_NORMAL_SIZE

/**** HELPERS FOR DYNAMIC TEXT ****/

/* Extra space to add after any text line */
#define TEXT_PADDING            2

/* Use this when the text extends to the right (e.g. checkboxes) */
#define LBLRIGHT(x, y, w, h, str) \
    (x), (y), (w) + gui_str_len(str) + TEXT_PADDING, (h), (str)

/* Use this when the space for the label is taken from the left (e.g. input) */
#define LBLLEFT(x, y, w, h, str) \
    (x) + (gui_str_len(str) + TEXT_PADDING), (y), \
    (w) - (gui_str_len(str) + TEXT_PADDING), (h), (str)

#endif
