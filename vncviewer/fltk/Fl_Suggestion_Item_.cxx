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

#include <list>

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>

#include "Fl_Suggestion_Item_.h"
#include "Fl_Suggestion_Input.h"

#define REMOVE_ITEM_MARGIN  4

Fl_Suggestion_Item_::Fl_Suggestion_Item_(
    int x, int y, int w, int h, const std::string label,
    Fl_Normalize_Callback *normalize_cb, Fl_Suggestion_Input *_parent
)
: Fl_Group(x, y, w, h), on_normalize_cb(normalize_cb), parent(_parent),
    full_label(label), selected(false)
{
    base_color = FL_BACKGROUND2_COLOR;
    base_hover_color = fl_color_average(FL_BLACK, base_color, 0.05);

    selected_color = FL_SELECTION_COLOR;
    selected_hover_color = fl_color_average(FL_BLACK, selected_color, 0.1);

    r_color = FL_DARK3;
    r_hover_color = FL_BLACK;
    r_selected_color = FL_DARK2;
    r_selected_hover_color = FL_WHITE;

    box(FL_NO_BOX);
    labelcolor(FL_FOREGROUND_COLOR);
    labelsize(FL_NORMAL_SIZE);
    align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    copy_label(label.c_str());

    select = new Fl_Box(x, y, w, h);
    select->box(FL_FLAT_BOX);
    select->color(base_color);
    select->clear_visible_focus();

    const int remove_size = h - (REMOVE_ITEM_MARGIN * 2);
    x += w - remove_size;
    y += (h - remove_size) / 2;

    // The remove-box will appear on top of the select-box
    remove = new Fl_Box(x, y, remove_size, remove_size);
    remove->box(FL_NO_BOX);
    remove->clear_visible_focus();
    remove->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    remove->copy_label("×");
    remove->labelfont(FL_BOLD);
    remove->labelcolor(r_color);
    remove->hide(); // Only show on hover

    end();
}

void Fl_Suggestion_Item_::set_highlight(const bool highlight)
{
    if (highlight) {
        selected = true;
        select->color(selected_color);
        remove->labelcolor(r_selected_color);
        labelcolor(FL_WHITE);
        damage(FL_DAMAGE_CHILD);
    } else {
        selected = false;
        select->color(base_color);
        remove->labelcolor(r_color);
        labelcolor(FL_FOREGROUND_COLOR);
        damage(FL_DAMAGE_CHILD);
    }
}

int Fl_Suggestion_Item_::handle(int event)
{
    switch (event) {
        case FL_ENTER: {
            select->color(selected ? selected_hover_color : base_hover_color);
            remove->show();
            damage(FL_DAMAGE_CHILD);
            return 1;
        }
        case FL_MOVE: {
            // Separate hover for the remove 'x'
            if (Fl::event_inside(remove))
                remove->labelcolor(selected ?
                                   r_selected_hover_color : r_hover_color);
            else if (Fl::event_inside(select))
                remove->labelcolor(selected ? r_selected_color : r_color);
            damage(FL_DAMAGE_ALL);
            return 1;
        }
        case FL_LEAVE: {
            select->color(selected ? selected_color : base_color);
            remove->labelcolor(selected ? r_selected_color : r_color);
            remove->hide();
            damage(FL_DAMAGE_CHILD);
            return 1;
        }
        case FL_HIDE: {
            remove->hide();
            break;
        }
        case FL_PUSH: {
            return 1;
        }
        case FL_RELEASE: {
            if (Fl::event_inside(remove)) {
                remove_item();
            } else {
                select_item();
            }
            return 1;
        }
        default: break;
    }
    return Fl_Group::handle(event);
}

bool Fl_Suggestion_Item_::is_match_first_or_longest(const Match first,
                                                    const Match second)
{
    // Sort by position then length
    if (first.pos == second.pos)
        return first.len > second.len;
    return first.pos < second.pos;
}
bool Fl_Suggestion_Item_::is_match_overlapping(const Match first,
                                               const Match second)
{
    // Check if the two matches overlap
    const int first_end = first.pos + first.len;
    const int second_end = second.pos + second.len;
    return first.pos < second_end && second.pos < first_end;
}

void Fl_Suggestion_Item_::draw()
{
    Fl_Group::draw_box();
    Fl_Group::draw_backdrop();
    Fl_Group::draw_children();

    fl_color(labelcolor());
    Fl_Font orig_font = labelfont();
    int orig_size = labelsize();

    int x = this->x() + Fl::box_dx(parent->box());
    int y = this->y() + (h() / 2) + (orig_size / 2);
    int offset = 0;

    if (matches.empty()) {
        fl_font(orig_font, orig_size);
        fl_draw(full_label.c_str(), x, y);
        return;
    }

    int start = 0;
    matches.sort(is_match_first_or_longest);
    matches.unique(is_match_overlapping);

    for (Match m : matches) {
        std::string before = full_label.substr(start, m.pos - start);
        std::string match = full_label.substr(m.pos, m.len);

        fl_font(orig_font, orig_size);
        fl_draw(before.c_str(), x + offset, y);
        offset += fl_width(before.c_str());

        fl_font(FL_BOLD, orig_size);
        fl_draw(match.c_str(), x + offset, y);
        offset += fl_width(match.c_str());

        start = m.pos + m.len;
    }

    std::string after = full_label.substr(start);

    fl_font(orig_font, orig_size);
    fl_draw(after.c_str(), x + offset, y);
    offset += fl_width(after.c_str());
}

void Fl_Suggestion_Item_::emphasize_match(const std::string match)
{
    matches.clear();
    if (match == "")
        return;

    std::string _match = on_normalize_cb(match);
    std::string _full_label = on_normalize_cb(full_label);

    std::list<std::string> words = Fl_Suggestion_Input::split_on_space(_match);
    if (words.empty())
        words.push_back(_match);

    for (std::string w : words) {
        std::string::size_type pos = _full_label.find(w);
        if (w != "" && pos != std::string::npos) {
            Match m;
            m.pos = pos;
            m.len = w.length();
            matches.push_back(m);
        }
    }
    damage(FL_DAMAGE_CHILD);
}

void Fl_Suggestion_Item_::select_item()
{
    parent->select_suggestion(label());
    parent->hide_suggestions();
}
void Fl_Suggestion_Item_::remove_item()
{
    parent->remove_suggestion(label());
}

