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
#include <sstream>
#include <algorithm>

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>

#include "Fl_Suggestion_Item_.h"
#include "Fl_Suggestion_Window_.h"
#include "Fl_Suggestion_Input.h"
#include "event_dispatch_handler.h"
#include "layout.h"

/* The item needs to be offset inside the dropdown window
   to not clip the border. On macOS the border is outside. */
#if defined(__APPLE__)
#define SUGGESTION_OFFSET  0
#else
#define SUGGESTION_OFFSET  1
#endif
#define SUGGESTION_ITEM_MARGIN  6
#define MAX_SHOWN_ITEMS  6

std::list<std::string> Fl_Suggestion_Input::split_on_space(const std::string s)
{
    std::list<std::string> result;
    std::istringstream stream_to_split(s);
    std::string word;
    while (stream_to_split >> word)
        result.push_back(word);
    return result;
}

Fl_Suggestion_Input::Fl_Suggestion_Input(
    int x, int y, int w, int h, const char *label,
    const std::list<std::string> &_suggestions
)
: Fl_Group(x, y, w, h, label), suggestions(_suggestions),
  searching(false)
{
    Fl_Group::color(FL_BACKGROUND2_COLOR);
    Fl_Group::box(FL_DOWN_BOX);
    align(FL_ALIGN_LEFT); // Place label outside, on the left side

    input = new Fl_Input(x + Fl::box_dx(box()), y + Fl::box_dy(box()),
                         w - Fl::box_dw(box()), h - Fl::box_dh(box()));
    input->box(FL_FLAT_BOX);
    input->color(FL_BACKGROUND2_COLOR);
    input->callback(text_changed_cb, this);
    input->when(FL_WHEN_CHANGED);

    max_width = w - 2 * SUGGESTION_OFFSET;
#if defined(__APPLE__)
    max_width -= 2; // macOS adds a thin border we need to account for
#endif

    dropdown_window = new Fl_Suggestion_Window_(this);

    end(); // Close Fl_Group

    suggestions.unique();
    hide_suggestions();

    // Fallback no-op normalizer
    on_normalize_cb = [](const std::string s) { return s; };

    // Some events need to be caught on the parent window, see below
    fl_add_event_dispatch(dispatch_helper, this);
}

Fl_Suggestion_Input::~Fl_Suggestion_Input()
{
    Fl::remove_idle(show_suggestions_soon, this);
    Fl::remove_timeout(hide_suggestions_soon, this);
    Fl::remove_idle(hide_suggestions_if_unfocused, this);
    Fl::remove_system_handler(handle_system_event);
    fl_remove_event_dispatch(dispatch_helper, this);
}

void Fl_Suggestion_Input::draw()
{
    Fl_Group::draw();

    // Draw the box taller to give the appearance of the rounded corners
    // disappearing when the dropdown window is shown. Without the rounded
    // bottom corners the dropdown visually sticks to the input in a nice way.
    if (suggestions_visible()) {
        draw_box(box(), x(), y(), w(), h() + Fl::box_dh(box()), color());

        // The taller box will appear on top of the input text.
        // We need redraw the input again to make it appear on top.
        Fl_Group::draw_child(*input);
    }
}

void Fl_Suggestion_Input::call_to_normalize(Fl_Normalize_Callback *cb)
{
    on_normalize_cb = cb;
}
void Fl_Suggestion_Input::call_on_remove(Fl_Suggestion_Callback *cb,
                                         void *data)
{
    on_remove_cb = cb;
    on_remove_data = data;
}

int Fl_Suggestion_Input::handle(int event)
{
    switch (event) {
        case FL_PUSH: {
            if (!dropdown_window->visible())
                // FIXME: Doing show_suggestions in next event loop cycle
                //        to avoid a focus bug on macOS:
                //        https://github.com/fltk/fltk/issues/1300
                Fl::add_idle(show_suggestions_soon, this);
            break;
        }
        case FL_UNFOCUS: {
            // Pressing suggestion items sometimes generates UNFOCUS events.
            // When this happens, other events might be in the queue. Let
            // them be handled first. FOCUS might be one of these events.
            Fl::add_idle(hide_suggestions_if_unfocused, this);
            break;
        }
        case FL_DEACTIVATE: {
            hide_suggestions();
            break;
        }
        case FL_KEYDOWN: {
            if (!dropdown_window->visible())
                break;

            if (Fl::event_key() == FL_Enter) {
                // If there's no highlight, use the first suggestion if it
                // has some match with the user input.
                if (highlight.empty() &&
                    length_of_word_matches(input->value(),
                                           shown_suggestions.front()) > 0)
                    highlight = get_next("");

                if (!highlight.empty())
                    select_suggestion(highlight);

                hide_suggestions();
                return 1;
            }
            break;
        }
        default: break;
    }
    return Fl_Group::handle(event);
}

int Fl_Suggestion_Input::fltkDispatch(int event, Fl_Window *)
{
    // Since a Fl_Input can't receive most keydown events, we have to look
    // for FL_Down & FL_Up on the parent window.
    //
    // We also want to close the dropdown window when the user presses 'ESC' or
    // clicks outside the widget. These two events need special handling for
    // different reasons:
    //
    // FLTK dialogs close on Escape. Their handling gets priority over it's
    // children's normal event handlers. If this widget is used inside a
    // dialog, the dialog itself will close when the user presses 'ESC'.
    //
    // This widget doesn't get mouse events when the user clicks outside the
    // widget itself. We can't listen to FL_UNFOCUS either since the user might
    // click some "dead space" in the parent window. This means that we have to
    // check for these events on the parent window.
    switch (event) {
        case FL_KEYDOWN:
            if (Fl::event_key() == FL_Escape && dropdown_window->visible()) {
                hide_suggestions();
                // Prevent the default action which might close dialogs etc.
                return 1;
            }

            if (Fl::focus() != input)
                break;

            // Open suggestions, or cycle to next/previous?
            if (!dropdown_window->visible()) {
                if (Fl::event_key() == FL_Down) {
                    show_suggestions();
                    return 1;
                }
            } else if (Fl::event_key() == FL_Down) {
                cycle_suggestion(DOWN);
                return 1;
            } else if (Fl::event_key() == FL_Up) {
                cycle_suggestion(UP);
                return 1;
            }
            break;
        case FL_PUSH:
            // Hide suggestions when clicking outside. We can't use
            // event_inside() for the dropdown since it's a separate window.
            if (dropdown_window->visible() &&
                !Fl::event_inside(this) &&
                !dropdown_window->is_inside(Fl::event_x_root(),
                                            Fl::event_y_root()))
                hide_suggestions();
            break;
        default: break;
    }
    return 0;
}

void Fl_Suggestion_Input::set_suggestions(
    const std::list<std::string> &_suggestions
)
{
    suggestions = _suggestions;
    suggestions.unique();
}
void Fl_Suggestion_Input::add_suggestion(const std::string s)
{
    if (s.empty())
        return;

    suggestions.remove(s); // Remove if it already exists
    suggestions.push_front(s);
}
void Fl_Suggestion_Input::remove_suggestion(const std::string s)
{
    if (std::find(suggestions.begin(),
                  suggestions.end(), s) != suggestions.end()) {
        suggestions.remove(s);
        search_suggestions(searching); // Do a new search if needed
        update_suggestions();
        if (on_remove_cb)
            on_remove_cb(this, s, on_remove_data);
    }
}

const char * Fl_Suggestion_Input::value() const
{
    return input->value();
}
int Fl_Suggestion_Input::value(const char *t)
{
    return input->value(t);
}
int Fl_Suggestion_Input::insert(const char *t, int l)
{
    // Fl_Input::insert() triggers the text_changed_cb, which we only want to
    // trigger when the user has typed something. Let's temporarily disable it.
    Fl_When saved_when = input->when();
    input->when(FL_WHEN_NEVER);

    int res = input->Fl_Input::insert(t, l);
    input->when(saved_when);

    return res;
}
void Fl_Suggestion_Input::position(int x, int y)
{
    Fl_Widget::position(x, y);
    input->Fl_Widget::position(x + Fl::box_dx(box()), y + Fl::box_dy(box()));
    // The dropdown is positioned on show()
}
int Fl_Suggestion_Input::take_focus()
{
    return input->take_focus();
}

void Fl_Suggestion_Input::show_suggestions()
{
    // Select current input if showing for the first time
    if (user_input.empty())
        select_suggestion(input->value());
    update_suggestions(); // This handles visibility
}

void Fl_Suggestion_Input::search_suggestions(const bool search)
{
    searching = search;
    sorted_suggestions = suggestions;
    if (search) {
        // Sorted by relevance, longest combined length of matched words first.
        // Keeps original order after it runs out of matching suggestions or if
        // there are no matches at all.
        auto number_of_matches = [this](const std::string first,
                                        const std::string second) {
            return length_of_word_matches(input->value(), first) >
                length_of_word_matches(input->value(), second);
        };
        sorted_suggestions.sort(number_of_matches);
    }
}
int Fl_Suggestion_Input::length_of_word_matches(
    const std::string needle, const std::string haystack
) const
{
    // Split input to make it possible to filter on separate words
    std::list<std::string> needle_words = split_on_space(
        on_normalize_cb(needle)
    );
    int n = 0;
    for (std::string w : needle_words) {
        if (on_normalize_cb(haystack).find(w) != std::string::npos)
            n += w.length();

        // Give exact matches priority
        if (w == on_normalize_cb(haystack))
            n++;
    }
    return n;
}

void Fl_Suggestion_Input::hide_suggestions()
{
    shown_suggestions.clear();
    dropdown_window->hide();
    highlight = "";

    // Mark parts in the main window around this widget for redrawing
    // to clean up the corner shenanigans of Fl_Suggestion_Input::draw()
    window()->damage(FL_DAMAGE_OVERLAY, x(), y(), w(), h() + Fl::box_dh(box()));
    Fl::remove_system_handler(handle_system_event);
}
bool Fl_Suggestion_Input::suggestions_visible() const
{
    return dropdown_window->visible();
}

void Fl_Suggestion_Input::create_item(const std::string label)
{
    const int height_of_prev = shown_suggestions.size() * item_height;

    Fl_Suggestion_Item_ * item = new Fl_Suggestion_Item_(
        SUGGESTION_OFFSET, SUGGESTION_OFFSET + height_of_prev,
        max_width, item_height, label, on_normalize_cb, this
    );
    if (searching)
        item->emphasize_match(user_input);

    item->set_highlight(label == highlight);

    dropdown_window->add(item);
    shown_suggestions.push_back(label);
}
void Fl_Suggestion_Input::dropdown_window_show()
{
    dropdown_window->show();

    // We want to draw the widget corners differently when dropdown is open,
    // so we need to mark for redrawing. See Fl_Suggestion_Input::draw()
    damage(FL_DAMAGE_OVERLAY);

    // Save window position, for move-detection
    last_x = window()->x();
    last_y = window()->y();
    Fl::add_system_handler(handle_system_event, this);
}

int Fl_Suggestion_Input::handle_system_event(void *event, void *data)
{
    Fl_Suggestion_Input *self = (Fl_Suggestion_Input*) data;

    // Since this dropdown window is separate it doesn't follow the parent
    // window when the user moves it. FLTK doesn't have any event for a window
    // being moved. We need to listen to system events to hide when moved.

#if defined(WIN32)
    // On Windows, interacting with the window frame starts a modal process
    // causing us to not get the expected WM_MOVE message. Fortunately, a
    // window move often starts with a mouse click on the title bar. This
    // generates a non-client (NC) message which we can react to.
    MSG *msg = (MSG*)event;
    if (msg->message == WM_NCLBUTTONDOWN ||
        msg->message == WM_NCRBUTTONDOWN ||
        msg->message == WM_NCMBUTTONDOWN)
        // We can't use add_idle here since the message loop is blocked,
        // as explained above. FLTK's timeouts do work fortunately.
        Fl::add_timeout(0, self->hide_suggestions_soon, self);
#else
    (void)event;
    // When not on Windows, we get system events for window moves. We can
    // avoid platform specific code by simply checking the position.
    if (self->window()->x() != self->last_x ||
        self->window()->y() != self->last_y)
        Fl::add_timeout(0, self->hide_suggestions_soon, self);
#endif
    return 0;
}

void Fl_Suggestion_Input::update_suggestions()
{
    // For each update we create a full set of new suggestion items, one for
    // each suggestion to show. Thus we need to delete the old ones first.
    //
    // This function is in the event handling call chain. That means we can't
    // do dropdown_window->clear() since it would immediately delete the
    // children, which could break further propagation of events.
    // Instead, we remove the children from the parent, and using delete_widget
    // we can schedule deletion at the next call to the event loop.
    while (dropdown_window->children()) {
        Fl_Widget * w = dropdown_window->child(0);
        dropdown_window->remove(w);
        Fl::delete_widget(w);
    }

    // Height of items depend on font size
    item_height = (SUGGESTION_ITEM_MARGIN * 2) + fl_size();

    // New item widgets for the suggestions to show
    shown_suggestions.clear();
    for (std::string s : sorted_suggestions) {

        // If not searching, don't show a suggestion matching the user input.
        // These are still available for the user, see cycle_suggestion().
        if (!searching && s == user_input)
            continue;

        if (shown_suggestions.size() >= MAX_SHOWN_ITEMS)
            break;

        create_item(s);
    }

    // Update visibility and size of dropdown window
    if (shown_suggestions.empty()) {
        hide_suggestions();
    } else {
        const int shown_height = shown_suggestions.size() * item_height;
        dropdown_window->size(max_width + SUGGESTION_OFFSET * 2,
                              shown_height + SUGGESTION_OFFSET * 2);
        if (!dropdown_window->visible())
            dropdown_window_show();
    }
    dropdown_window->damage(FL_DAMAGE_CHILD);
}

void Fl_Suggestion_Input::cycle_suggestion(const Direction direction)
{
    if (shown_suggestions.empty())
        return;

    // Find index of highlighted item
    int highlight_idx = 0;
    for (std::string s : shown_suggestions) {
        if (s == highlight)
            break;
        highlight_idx++;
    }

    // Either cycle to the user input text or highlight next suggestion. The
    // user's text input is available at the top "before" the suggestions if
    // moving upwards (previous), or at the bottom "after" the suggestions if
    // moving downwards (next).
    const bool at_top = highlight_idx == 0;
    const bool at_bottom = highlight_idx == (int)shown_suggestions.size() - 1;

    if (!highlight.empty() && ((at_top && direction == UP) ||
                               (at_bottom && direction == DOWN))) {
        input->value(user_input.c_str());
        highlight = "";
        update_suggestions();
    } else {
        if (direction == UP)
            highlight = get_previous(highlight);
        else
            highlight = get_next(highlight);
        input->value(highlight.c_str());
        update_suggestions();
    }
}
std::string Fl_Suggestion_Input::get_next(const std::string current) const
{
    auto iter = std::find(shown_suggestions.begin(), shown_suggestions.end(),
                          current);

    if (iter != shown_suggestions.end())
        ++iter;

    // Wrapping at the end of the list
    if (iter == shown_suggestions.end())
        iter = shown_suggestions.begin();

    return *iter;
}
std::string Fl_Suggestion_Input::get_previous(const std::string current) const
{
    auto riter = std::find(shown_suggestions.rbegin(), shown_suggestions.rend(),
                           current);

    if (riter != shown_suggestions.rend())
        ++riter;

    // Wrapping at the start of the list
    if (riter == shown_suggestions.rend())
        riter = shown_suggestions.rbegin();

    return *riter;
}
void Fl_Suggestion_Input::select_suggestion(const std::string selected)
{
    user_input = selected;
    search_suggestions(false);

    input->value(selected.c_str());
}

void Fl_Suggestion_Input::text_changed_cb(Fl_Widget *, void *w)
{
    Fl_Suggestion_Input *self = (Fl_Suggestion_Input*) w;
    self->highlight = "";
    self->user_input = std::string(self->input->value());

    // Change behavior slightly if user has typed something, sort
    // suggestions depending on the input.
    self->search_suggestions(true);
    self->update_suggestions();
}

void Fl_Suggestion_Input::show_suggestions_soon(void *data)
{
    Fl::remove_idle(show_suggestions_soon, data);

    Fl_Suggestion_Input *self = (Fl_Suggestion_Input*) data;
    self->show_suggestions();
}

void Fl_Suggestion_Input::hide_suggestions_soon(void *data)
{
    // See handle_system_event for why timeout instead of idle
    Fl::remove_timeout(hide_suggestions_soon, data);

    Fl_Suggestion_Input *self = (Fl_Suggestion_Input*) data;
    self->hide_suggestions();
}

void Fl_Suggestion_Input::hide_suggestions_if_unfocused(void *data)
{
    Fl::remove_idle(hide_suggestions_if_unfocused, data);

    Fl_Suggestion_Input *self = (Fl_Suggestion_Input*) data;

    // Hide the suggestions if we're not focused
    if (Fl::focus() != self->input && Fl::focus() != self->dropdown_window)
        self->hide_suggestions();

    // The dropdown window might get focus, we want the input to be
    // focused for keyboard input.
    if (Fl::focus() == self->dropdown_window)
        self->take_focus();
}

int Fl_Suggestion_Input::dispatch_helper(int event, Fl_Window *w, void *data)
{
    Fl_Suggestion_Input *self = (Fl_Suggestion_Input*) data;
    return self->fltkDispatch(event, w);
}

