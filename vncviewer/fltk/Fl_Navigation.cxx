/* Copyright 2022 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>

#include "Fl_Navigation.h"

Fl_Navigation::Fl_Navigation(int x, int y, int w, int h)
  : Fl_Group(x, y, w, h)
{
  int dummy;

  scroll = new Fl_Scroll(x, y, 1, 1);
  scroll->type(Fl_Scroll::VERTICAL);
  scroll->color(FL_BACKGROUND2_COLOR);

  labels = new Fl_Group(x, y, 1, 1);
  labels->end();

  scroll->end();

  pages = new Fl_Group(x, y, 1, 1);
  pages->end();

  Fl_Group::end();

  // Just to resize things, and avoid code duplication
  client_area(dummy, dummy, dummy, dummy, w/10);

  begin();
}

Fl_Navigation::~Fl_Navigation()
{
}

Fl_Widget *Fl_Navigation::value()
{
  int i;

  for (i = 0;i < pages->children();i++) {
    if (pages->child(i)->visible())
      return pages->child(i);
  }

  return nullptr;
}

int Fl_Navigation::value(Fl_Widget *newpage)
{
  int i;
  int found;

  assert(labels->children() == pages->children());

  found = 0;
  for (i = 0;i < pages->children();i++) {
    if (pages->child(i) == newpage) {
      pages->child(i)->show();
      ((Fl_Button*)labels->child(i))->setonly();
      found = 1;
    } else {
      pages->child(i)->hide();
    }
  }

  return found;
}

void Fl_Navigation::client_area(int &rx, int &ry,
                                int &rw, int &rh, int lw)
{
  if (!pages->children()) {
    int cx, cy, cw, ch;

    cx = x() + 1;
    cy = y() + 1;
    cw = w() - 2;
    ch = h() - 2;

    scroll->resize(cx, cy, lw, ch);
    labels->resize(cx, cy, lw, ch);
    pages->resize(cx + lw + 1, cy, cw - lw - 1, ch);
  }

  rx = pages->x();
  ry = pages->y();
  rw = pages->w();
  rh = pages->h();
}

void Fl_Navigation::draw()
{
  draw_box(FL_BORDER_FRAME, x(), y(),
           labels->w()+2, h(), FL_DARK3);
  draw_box(FL_BORDER_FRAME, x()+1+labels->w(), y(),
           w() - (labels->w()+1), h(), FL_DARK3);
  Fl_Group::draw();
}

void Fl_Navigation::begin()
{
  pages->begin();
}

void Fl_Navigation::end()
{
  pages->end();
  Fl_Group::end();

  update_labels();
}

void Fl_Navigation::update_labels()
{
  int i, offset;

  labels->clear();
  labels->resizable(nullptr);

  if (!pages->children())
    return;

  offset = 0;
  for (i = 0;i < pages->children();i++) {
    Fl_Widget *page;
    Fl_Button *btn;

    page = pages->child(i);

    btn = new Fl_Button(labels->x(), labels->y() + offset,
                        labels->w(), page->labelsize() * 3,
                        page->label());
    btn->box(FL_FLAT_BOX);
    btn->type(FL_RADIO_BUTTON);
    btn->color(FL_BACKGROUND2_COLOR);
    btn->selection_color(FL_SELECTION_COLOR);
    btn->labelsize(page->labelsize());
    btn->labelfont(page->labelfont());
    btn->image(page->image());
    btn->deimage(page->deimage());
    btn->callback(label_pressed, this);

    labels->add(btn);
    offset += page->labelsize() * 3;
  }
  labels->size(labels->w(), offset);

  // FIXME: Retain selection
  value(pages->child(0));
}

void Fl_Navigation::label_pressed(Fl_Widget *widget, void *user_data)
{
  Fl_Navigation *self = (Fl_Navigation *) user_data;

  int i, idx;

  idx = -1;
  for (i = 0;i < self->labels->children();i++) {
    if (self->labels->child(i) == widget)
      idx = i;
  }

  assert(idx >= 0);
  assert(idx < self->pages->children());

  self->value(self->pages->child(idx));
}
