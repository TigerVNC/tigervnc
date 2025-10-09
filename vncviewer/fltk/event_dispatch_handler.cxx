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
#include <algorithm>
#include <FL/Fl.H>

#include "event_dispatch_handler.h"

struct event_dispatch_handler {
    Fl_Event_Dispatch_Handler handler;
    void *data;
};
static bool operator==(const event_dispatch_handler& a,
                       const event_dispatch_handler& b)
{
    return a.handler == b.handler && a.data == b.data;
}

static std::list<event_dispatch_handler> handlers;

static int call_handlers(int event, Fl_Window *w)
{
    // Convoluted iteration so that we safely handle changes to the list
    std::list<event_dispatch_handler> handlers_copy = handlers;
    for (event_dispatch_handler l : handlers_copy) {
        if (std::find(handlers.begin(), handlers.end(), l) == handlers.end())
            continue;
        if(l.handler(event, w, l.data))
            return 1;
    }
    return Fl::handle_(event, w);
}

void fl_add_event_dispatch(Fl_Event_Dispatch_Handler ha, void *data)
{
    if (handlers.empty())
        Fl::event_dispatch(call_handlers);
    handlers.push_back({ha, data});
}

void fl_remove_event_dispatch(Fl_Event_Dispatch_Handler ha, void *data)
{
    std::list<event_dispatch_handler>::iterator iter;
    for (iter = handlers.begin(); iter != handlers.end();) {
        if ((iter->handler == ha) && (iter->data == data))
            iter = handlers.erase(iter);
        else
            ++iter;
    }
    if (handlers.empty())
        Fl::event_dispatch(Fl::handle_);
}
