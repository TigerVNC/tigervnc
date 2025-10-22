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

#ifndef __SUGGESTION_INPUT_H__
#define __SUGGESTION_INPUT_H__

#include <list>
#include <string>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>

class Fl_Input;
class Fl_Suggestion_Item_;
class Fl_Suggestion_Window_;

typedef std::string (Fl_Normalize_Callback)(const std::string s);
typedef void (Fl_Suggestion_Callback)(Fl_Widget*, std::string s, void*);

class Fl_Suggestion_Input: public Fl_Group {
public:
    Fl_Suggestion_Input(int x, int y, int w, int h, const char *label,
                        const std::list<std::string> &_suggestions = {});
    ~Fl_Suggestion_Input();
    void call_to_normalize(Fl_Normalize_Callback *cb);
    void call_on_remove(Fl_Suggestion_Callback *cb, void *data);

    const char * value() const;
    int value(const char * t);
    int insert(const char * t, int l=0);
    void position(int x, int y);
    int take_focus();

    void set_suggestions(const std::list<std::string> &_suggestions);
    void add_suggestion(const std::string s);
    void remove_suggestion(const std::string s);
    void show_suggestions();
    void hide_suggestions();
    bool suggestions_visible() const;

    static std::list<std::string> split_on_space(const std::string s);

private:
    friend Fl_Suggestion_Item_;
    std::list<std::string> suggestions;
    std::list<std::string> sorted_suggestions;
    std::list<std::string> shown_suggestions;
    Fl_Input * input;
    Fl_Suggestion_Window_ *dropdown_window;

    Fl_Normalize_Callback * on_normalize_cb;
    Fl_Suggestion_Callback * on_remove_cb;
    void * on_remove_data;

    std::string highlight;
    std::string user_input;
    bool searching;
    int max_width;
    int item_height;

    int last_x;
    int last_y;

    int fltkDispatch(int event, Fl_Window *);
    virtual void draw() override;
    virtual int handle(int event) override;

    void search_suggestions(const bool search);
    int length_of_word_matches(const std::string needle,
                               const std::string haystack) const;
    void create_item(const std::string label);
    void dropdown_window_show();
    static int handle_system_event(void *event, void *data);
    void update_suggestions();

    enum Direction { UP, DOWN };
    void cycle_suggestion(const Direction direction);
    std::string get_next(const std::string current) const;
    std::string get_previous(const std::string current) const;
    void select_suggestion(const std::string selected);

    static void text_changed_cb(Fl_Widget *, void *w);
    static void show_suggestions_soon(void *data);
    static void hide_suggestions_soon(void *data);
    static void hide_suggestions_if_unfocused(void *data);
    static int dispatch_helper(int event, Fl_Window *w, void *data);
};

#endif
