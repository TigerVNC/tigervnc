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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <sys/time.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#include <rdr/Exception.h>
#include <rfb/util.h>

#include "../vncviewer/PlatformPixelBuffer.h"

#include "util.h"

class TestWindow: public Fl_Window {
public:
  TestWindow();
  ~TestWindow();

  virtual void start(int width, int height);
  virtual void stop();

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

class OverlayTestWindow: public PartialTestWindow {
public:
  OverlayTestWindow();

  virtual void start(int width, int height);
  virtual void stop();

  virtual void draw();

protected:
  Surface* overlay;
  Surface* offscreen;
};

TestWindow::TestWindow() :
  Fl_Window(0, 0, "Framebuffer Performance Test"),
  fb(NULL)
{
}

TestWindow::~TestWindow()
{
  stop();
}

void TestWindow::start(int width, int height)
{
  rdr::U32 pixel;

  stop();

  resize(x(), y(), width, height);

  pixels = 0;
  frames = 0;
  time = 0;

  fb = new PlatformPixelBuffer(w(), h());

  pixel = 0;
  fb->fillRect(fb->getRect(), &pixel);

  show();
}

void TestWindow::stop()
{
  hide();

  delete fb;
  fb = NULL;

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

OverlayTestWindow::OverlayTestWindow() :
  overlay(NULL), offscreen(NULL)
{
}

void OverlayTestWindow::start(int width, int height)
{
  PartialTestWindow::start(width, height);

  overlay = new Surface(400, 200);
  overlay->clear(0xff, 0x80, 0x00, 0xcc);

  // X11 needs an off screen buffer for compositing to avoid flicker,
  // and alpha blending doesn't work for windows on Win32
#if !defined(__APPLE__)
  offscreen = new Surface(w(), h());
#else
  offscreen = NULL;
#endif
}

void OverlayTestWindow::stop()
{
  PartialTestWindow::stop();

  delete offscreen;
  offscreen = NULL;
  delete overlay;
  overlay = NULL;
}

void OverlayTestWindow::draw()
{
  int ox, oy, ow, oh;
  int X, Y, W, H;

  // We cannot update the damage region from inside the draw function,
  // so delegate this to an idle function
  Fl::add_idle(timer, this);

  // Check what actually needs updating
  fl_clip_box(0, 0, w(), h(), X, Y, W, H);
  if ((W == 0) || (H == 0))
    return;

  // We might get a redraw before we are fully ready
  if (!overlay)
    return;

  // Simplify the clip region to a simple rectangle in order to
  // properly draw all the layers even if they only partially overlap
  fl_push_no_clip();
  fl_push_clip(X, Y, W, H);

  if (offscreen)
    fb->draw(offscreen, X, Y, X, Y, W, H);
  else
    fb->draw(X, Y, X, Y, W, H);

  pixels += W*H;
  frames++;

  ox = (w() - overlay->width()) / 2;
  oy = h() / 4 - overlay->height() / 2;
  ow = overlay->width();
  oh = overlay->height();
  fl_clip_box(ox, oy, ow, oh, X, Y, W, H);
  if ((W != 0) && (H != 0)) {
    if (offscreen)
      overlay->blend(offscreen, X - ox, Y - oy, X, Y, W, H);
    else
      overlay->blend(X - ox, Y - oy, X, Y, W, H);
  }

  fl_pop_clip();
  fl_pop_clip();

  if (offscreen) {
    fl_clip_box(0, 0, w(), h(), X, Y, W, H);
    offscreen->draw(X, Y, X, Y, W, H);
  }
}

static void dosubtest(TestWindow* win, int width, int height,
                      unsigned long long* pixels,
		      unsigned long long* frames,
		      double* time)
{
  struct timeval start;

  win->start(width, height);

  gettimeofday(&start, NULL);
  while (rfb::msSince(&start) < 3000)
    Fl::wait();

  win->stop();

  *pixels = win->pixels;
  *frames = win->frames;
  *time = win->time;
}

static bool is_constant(double a, double b)
{
    return (fabs(a - b) / a) < 0.1;
}

static void dotest(TestWindow* win)
{
  unsigned long long pixels[3];
  unsigned long long frames[3];
  double time[3];

  double delay, rate;
  char s[1024];

  // Run the test several times at different resolutions...
  dosubtest(win, 800, 600, &pixels[0], &frames[0], &time[0]);
  dosubtest(win, 1024, 768, &pixels[1], &frames[1], &time[1]);
  dosubtest(win, 1280, 960, &pixels[2], &frames[2], &time[2]);

  // ...in order to compute how much of the rendering time is static,
  // and how much depends on the number of pixels
  // (i.e. solve: time = delay * frames + rate * pixels)
  delay = (((time[0] - (double)pixels[0] / pixels[1] * time[1]) /
            (frames[0] - (double)pixels[0] / pixels[1] * frames[1])) +
           ((time[1] - (double)pixels[1] / pixels[2] * time[2]) /
            (frames[1] - (double)pixels[1] / pixels[2] * frames[2]))) / 2.0;
  rate = (((time[0] - (double)frames[0] / frames[1] * time[1]) /
           (pixels[0] - (double)frames[0] / frames[1] * pixels[1])) +
          ((time[1] - (double)frames[1] / frames[2] * time[2]) /
           (pixels[1] - (double)frames[1] / frames[2] * pixels[2]))) / 2.0;

  // However, we have some corner cases:

  // We are restricted by some delay, e.g. refresh rate
  if (is_constant(frames[0]/time[0], frames[2]/time[2])) {
    fprintf(stderr, "WARNING: Fixed delay dominating updates.\n\n");
    delay = time[2]/frames[2];
    rate = 0.0;
  }

  // There isn't any fixed delay, we are only restricted by pixel
  // throughput
  if (fabs(delay) < 0.001) {
    delay = 0.0;
    rate = time[2]/pixels[2];
  }

  // We can hit cache limits that causes performance to drop
  // with increasing update size, screwing up our calculations
  if ((pixels[2] / time[2]) < (pixels[0] / time[0] * 0.9)) {
    fprintf(stderr, "WARNING: Unexpected behaviour. Measurement unreliable.\n\n");

    // We can't determine the proportions between these, so divide the
    // time spent evenly
    delay = time[2] / 2.0 / frames[2];
    rate = time[2] / 2.0 / pixels[2];
  }

  fprintf(stderr, "Rendering delay: %g ms/frame\n", delay * 1000.0);
  if (rate == 0.0)
    strcpy(s, "N/A pixels/s");
  else
    rfb::siPrefix(1.0 / rate, "pixels/s", s, sizeof(s));
  fprintf(stderr, "Rendering rate: %s\n", s);
  fprintf(stderr, "Maximum FPS: %g fps @ 1920x1080\n",
          1.0 / (delay + rate * 1920 * 1080));
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

  fprintf(stderr, "Partial window update with overlay:\n\n");
  win = new OverlayTestWindow();
  dotest(win);
  delete win;
  fprintf(stderr, "\n");

  return 0;
}
