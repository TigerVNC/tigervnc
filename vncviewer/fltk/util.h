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

#ifndef __FLTK_UTIL_H__
#define __FLTK_UTIL_H__

#include <FL/Fl_Menu_.H>

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
                                 void *data=nullptr, int flags=0)
{
    char buffer[1024];

    if (fltk_menu_escape(text, buffer, sizeof(buffer)) >= sizeof(buffer))
        return;

    menu->add(buffer, shortcut, cb, data, flags);
}

#endif
