/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __FLTK_LAYOUT_H__
#define __FLTK_LAYOUT_H__

#include <FL/fl_draw.H>
#include <FL/Fl_Menu_.H>

/* Calculates the width of a string as printed by FLTK (pixels) */
static inline int gui_str_len(const char *str)
{
    float len;

    fl_font(FL_HELVETICA, FL_NORMAL_SIZE);

    len = fl_width(str);

    return (int)(len + 0.5f);
}

/* Escapes all @ in text as those have special meaning in labels */
static inline size_t fltk_escape(const char *in, char *out, size_t maxlen)
{
    size_t len;

    len = 0;

    while (*in != '\0') {
        if (*in == '@') {
            if (maxlen >= 3) {
                *out++ = '@';
                *out++ = '@';
                maxlen -= 2;
            }

            len += 2;
        } else {
            if (maxlen >= 2) {
                *out++ = *in;
                maxlen--;
            }

            len += 1;
        }

        in++;
    }

    if (maxlen)
        *out = '\0';

    return len;
}

/* Filter out unsafe characters for menu entries */
static inline size_t fltk_menu_escape(const char *in, char *out, size_t maxlen)
{
    size_t len;

    len = 0;

    while (*in != '\0') {
        if (*in == '/') {
            if (maxlen >= 3) {
                *out++ = '\\';
                *out++ = '/';
                maxlen -= 2;
            }

            len += 2;
        } else {
            if (maxlen >= 2) {
                *out++ = *in;
                maxlen--;
            }

            len += 1;
        }

        in++;
    }

    if (maxlen)
        *out = '\0';

    return len;
}

/* Helper to add menu entries safely */
static inline void fltk_menu_add(Fl_Menu_ *menu, const char *text,
                                 int shortcut, Fl_Callback *cb,
                                 void *data = 0, int flags = 0)
{
    char buffer[1024];

    if (fltk_menu_escape(text, buffer, sizeof(buffer)) >= sizeof(buffer))
        return;

    menu->add(buffer, shortcut, cb, data, flags);
}

/**** MARGINS ****/

#define OUTER_MARGIN            10
#define INNER_MARGIN            10

/* Tighter grouping of related fields */
#define TIGHT_MARGIN            5

/**** ADJUSTMENTS ****/
#define INDENT                  20

/**** FLTK WIDGETS ****/

/* Fl_Tabs */
#define TABS_HEIGHT             30

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
#define GROUP_MARGIN            12

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
