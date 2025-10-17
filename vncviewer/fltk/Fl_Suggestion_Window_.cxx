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

#include <FL/Fl.H>
#include <FL/Fl_Menu_Window.H>

#include "Fl_Suggestion_Window_.h"
#include "Fl_Suggestion_Input.h"

Fl_Suggestion_Window_::Fl_Suggestion_Window_(Fl_Suggestion_Input *_widget)
: Fl_Menu_Window(0, 0), widget(_widget)
{
    set_override(); // Show on top of the parent (also hides borders)
    set_non_modal(); // Keep always on top
    force_position(1); // Don't let the window move
#if !defined(__APPLE__) // macOS adds a thin border by itself
    color(fl_color_average(FL_BLACK, FL_BACKGROUND_COLOR, 0.15)); // Border
#endif
    end();
}

void Fl_Suggestion_Window_::show()
{
    // Place the dropdown below the input
    int x = widget->x();
    int y = widget->y() + widget->h();

#if defined(__APPLE__)
    x += 1; // macOS adds a thin window border we need to account for
#endif

    // Since it's a separate window, we need use screen coordinates
    int win_x = widget->window()->x();
    int win_y = widget->window()->y();

    position(win_x + x, win_y + y);

    Fl_Menu_Window::show();
}

void Fl_Suggestion_Window_::size(const int w, const int h)
{
    int work_x, work_y, work_w, work_h;
    Fl::screen_work_area(work_x, work_y, work_w, work_h,
                         Fl::screen_num(x(), y()));

    // Make sure the window isn't taller than fits in the work area
    int root_y = widget->window()->y() + widget->y() + widget->h();
    int new_h = h;
    if (root_y + h > work_y + work_h)
        new_h = work_y + work_h - root_y;

    Fl_Menu_Window::size(w, new_h);
}

bool Fl_Suggestion_Window_::is_inside(const int root_x, const int root_y) const
{
    return (root_x > x() && root_x < (x() + w()) &&
            root_y > y() && root_y < (y() + h()));
}

