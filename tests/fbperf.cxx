/* Copyright 2016 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <sys/time.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include <rdr/Exception.h>
#include <rfb/util.h>

#include "../vncviewer/PlatformPixelBuffer.h"
#include "../vncviewer/FLTKPixelBuffer.h"

#if defined(WIN32)
#include "../vncviewer/Win32PixelBuffer.h"
#elif defined(__APPLE__)
#include "../vncviewer/OSXPixelBuffer.h"
#else
#include "../vncviewer/X11PixelBuffer.h"
#endif

#include "util.h"

class TestWindow: public Fl_Window {
public:
  TestWindow();
  ~TestWindow();

  virtual void draw();

protected:
  virtual void flush();

  void update();
  virtual void changefb();

  static void timer(void* data);

public:
  unsigned long long pixels, frames;
  double time;

protected:
  PlatformPixelBuffer* fb;
};

class PartialTestWindow: public TestWindow {
protected:
  virtual void changefb();
};

TestWindow::TestWindow() :
  Fl_Window(1024, 768, "Framebuffer Performance Test")
{
  rdr::U32 pixel;

  pixels = 0;
  frames = 0;
  time = 0;

  try {
#if defined(WIN32)
    fb = new Win32PixelBuffer(w(), h());
#elif defined(__APPLE__)
    fb = new OSXPixelBuffer(w(), h());
#else
    fb = new X11PixelBuffer(w(), h());
#endif
  } catch (rdr::Exception& e) {
    fb = new FLTKPixelBuffer(w(), h());
  }

  pixel = 0;
  fb->fillRect(fb->getRect(), &pixel);
}

TestWindow::~TestWindow()
{
  delete fb;

  Fl::remove_idle(timer, this);
}

void TestWindow::draw()
{
  int X, Y, W, H;

  // We cannot update the damage region from inside the draw function,
  // so delegate this to an idle function
  Fl::add_idle(timer, this);

  // Check what actually needs updating
  fl_clip_box(0, 0, w(), h(), X, Y, W, H);
  if ((W == 0) || (H == 0))
    return;

  fb->draw(X, Y, X, Y, W, H);

  pixels += W*H;
  frames++;
}

void TestWindow::flush()
{
  startTimeCounter();
  Fl_Window::flush();
#if !defined(WIN32) && !defined(__APPLE__)
  // Make sure we measure any work we queue up
  XSync(fl_display, False);
#endif
  endTimeCounter();

  time += getTimeCounter();
}

void TestWindow::update()
{
  rfb::Rect r;

  startTimeCounter();

  changefb();

  r = fb->getDamage();
  damage(FL_DAMAGE_USER1, r.tl.x, r.tl.y, r.width(), r.height());

#if !defined(WIN32) && !defined(__APPLE__)
  // Make sure we measure any work we queue up
  XSync(fl_display, False);
#endif

  endTimeCounter();

  time += getTimeCounter();
}

void TestWindow::changefb()
{
  rdr::U32 pixel;

  pixel = rand();
  fb->fillRect(fb->getRect(), &pixel);
}

void TestWindow::timer(void* data)
{
  TestWindow* self;

  Fl::remove_idle(timer, data);

  self = (TestWindow*)data;
  self->update();
}

void PartialTestWindow::changefb()
{
  rfb::Rect r;
  rdr::U32 pixel;

  r = fb->getRect();
  r.tl.x += w() / 4;
  r.tl.y += h() / 4;
  r.br.x -= w() / 4;
  r.br.y -= h() / 4;

  pixel = rand();
  fb->fillRect(r, &pixel);
}

static void dotest(TestWindow* win)
{
  struct timeval start;
  char s[1024];

  win->show();

  gettimeofday(&start, NULL);
  while (rfb::msSince(&start) < 10000)
    Fl::wait();

  fprintf(stderr, "Rendering time: %g ms/frame\n",
          win->time * 1000.0 / win->frames);
  rfb::siPrefix(win->pixels / win->time,
                "pixels/s", s, sizeof(s));
  fprintf(stderr, "Rendering rate: %s\n", s);
}

int main(int argc, char** argv)
{
  TestWindow* win;

  fprintf(stderr, "Full window update:\n\n");
  win = new TestWindow();
  dotest(win);
  delete win;
  fprintf(stderr, "\n");

  fprintf(stderr, "Partial window update:\n\n");
  win = new PartialTestWindow();
  dotest(win);
  delete win;
  fprintf(stderr, "\n");

  return 0;
}
