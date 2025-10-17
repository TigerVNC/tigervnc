/* Copyright 2025 Samuel Mannehed for Cendio AB
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

#ifndef __SUGGESTION_ITEM_H__
#define __SUGGESTION_ITEM_H__

#include <list>
#include <string>
#include <FL/Fl_Group.H>

#include "Fl_Suggestion_Input.h"

class Fl_Box;

// This is a helper class for Fl_Suggestion_Input

class Fl_Suggestion_Item_: public Fl_Group {
public:
    Fl_Suggestion_Item_(int x, int y, int w, int h,
                        const std::string label,
                        Fl_Normalize_Callback *normalize_cb,
                        Fl_Suggestion_Input *parent);

    const char * label() const { return full_label.c_str(); };
    void set_highlight(const bool highlight);
    void emphasize_match(const std::string match);

private:
    Fl_Box *select;
    Fl_Box *remove;
    Fl_Normalize_Callback * on_normalize_cb;
    Fl_Suggestion_Input *parent;

    std::string full_label;
    bool selected;

    int base_color;
    int base_hover_color;
    int selected_color;
    int selected_hover_color;
    int r_color;
    int r_hover_color;
    int r_selected_color;
    int r_selected_hover_color;

    typedef struct Match {
        int pos;
        int len;
    } Match;
    std::list<Match> matches;

    void select_item();
    void remove_item();

    static bool is_match_first_or_longest(const Match first,
                                          const Match second);
    static bool is_match_overlapping(const Match first,
                                     const Match second);

    virtual int handle(int event) override;
    virtual void draw() override;
};

#endif
