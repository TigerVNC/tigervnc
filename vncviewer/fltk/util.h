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

#include <string>

#include <FL/Fl_Menu_.H>

/* Escapes all @ in text as those have special meaning in labels */
static inline std::string fltk_escape(const std::string& in)
{
    std::string out;

    for (char ch : in) {
        if (ch == '@')
            out.append("@@");
        else
            out.append(1, ch);
    }

    return out;
}

/* Filter out unsafe characters for menu entries */
static inline std::string fltk_menu_escape(const std::string& in)
{
    std::string out;

    for (char ch : in) {
        if (ch == '/')
            out.append("\\/");
        else
            out.append(1, ch);
    }

    return out;
}

/* Helper to add menu entries safely */
static inline void fltk_menu_add(Fl_Menu_ *menu, const char *text,
                                 int shortcut, Fl_Callback *cb,
                                 void *data=nullptr, int flags=0)
{
    menu->add(fltk_menu_escape(text).c_str(), shortcut, cb, data, flags);
}

#endif
