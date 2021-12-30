/* Copyright 2020 Alex Tanskanen <aleta@cendio.se> for Cendio AB
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

#include <stdio.h>
#include <vector>
#include <unistd.h>

#include <rfb/Rect.h>
#include <rfb/Configuration.h>
#include "EmulateMB.h"

// The button masks for the mouse buttons
static const int empty          = 0x00;
static const int left           = 0x01;
static const int middle         = 0x02;
static const int middleAndLeft  = 0x03;
static const int right          = 0x04;
static const int both           = 0x05;
static const int middleAndRight = 0x06;

rfb::BoolParameter emulateMiddleButton("dummy_name", "dummy_desc", true);

class TestClass : public EmulateMB
{
public:
  virtual void sendPointerEvent(const rfb::Point& pos, int buttonMask);

  struct PointerEventParams {rfb::Point pos; int mask; };

  std::vector<PointerEventParams> results;
};

void TestClass::sendPointerEvent(const rfb::Point& pos, int buttonMask)
{
  PointerEventParams params;
  params.pos = pos;
  params.mask = buttonMask;
  results.push_back(params);
}

#define ASSERT_EQ(expr, val) if ((expr) != (val)) { \
  printf("FAILED on line %d (%s equals %d, expected %d)\n", __LINE__, #expr, (int)(expr), (int)(val)); \
  return; \
}

void testDisabledOption()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(false);
  test.filterPointerEvent(rfb::Point(0, 10), left);

  ASSERT_EQ(test.results.size(), 1);

  ASSERT_EQ(test.results[0].pos.x, 0);
  ASSERT_EQ(test.results[0].pos.y, 10);
  ASSERT_EQ(test.results[0].mask, left);

  printf("OK\n");
}

void testLeftClick()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(0, 0), left);
  test.filterPointerEvent(rfb::Point(0, 0), empty);

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 0);
  ASSERT_EQ(test.results[0].pos.y, 0);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 0);
  ASSERT_EQ(test.results[1].pos.y, 0);
  ASSERT_EQ(test.results[1].mask, left);

  ASSERT_EQ(test.results[2].pos.x, 0);
  ASSERT_EQ(test.results[2].pos.y, 0);
  ASSERT_EQ(test.results[2].mask, empty);

  printf("OK\n");
}

void testNormalLeftPress()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(10, 20), left);
  usleep(100000); // 0.1s
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.results.size(), 2);

  ASSERT_EQ(test.results[0].pos.x, 10);
  ASSERT_EQ(test.results[0].pos.y, 20);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 10);
  ASSERT_EQ(test.results[1].pos.y, 20);
  ASSERT_EQ(test.results[1].mask, left);

  printf("OK\n");
}

void testNormalMiddlePress()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(0, 0), middle);

  ASSERT_EQ(test.results.size(), 1);

  ASSERT_EQ(test.results[0].pos.x, 0);
  ASSERT_EQ(test.results[0].pos.y, 0);
  ASSERT_EQ(test.results[0].mask, middle);

  printf("OK\n");
}

void testNormalRightPress()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(0, 0), right);
  usleep(100000); // 0.1s
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.results.size(), 2);

  ASSERT_EQ(test.results[0].pos.x, 0);
  ASSERT_EQ(test.results[0].pos.y, 0);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 0);
  ASSERT_EQ(test.results[1].pos.y, 0);
  ASSERT_EQ(test.results[1].mask, right);

  printf("OK\n");
}

void testEmulateMiddleMouseButton()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(20, 30), right);
  test.filterPointerEvent(rfb::Point(20, 30), both);

  ASSERT_EQ(test.results.size(), 2);

  ASSERT_EQ(test.results[0].pos.x, 20);
  ASSERT_EQ(test.results[0].pos.y, 30);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 20);
  ASSERT_EQ(test.results[1].pos.y, 30);
  ASSERT_EQ(test.results[1].mask, middle);

  printf("OK\n");
}

void testLeftReleaseAfterEmulate()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(20, 30), left);
  test.filterPointerEvent(rfb::Point(20, 30), both);
  test.filterPointerEvent(rfb::Point(20, 30), right); // left released

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 20);
  ASSERT_EQ(test.results[0].pos.y, 30);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 20);
  ASSERT_EQ(test.results[1].pos.y, 30);
  ASSERT_EQ(test.results[1].mask, middle);

  ASSERT_EQ(test.results[2].pos.x, 20);
  ASSERT_EQ(test.results[2].pos.y, 30);
  ASSERT_EQ(test.results[2].mask, middle);

  printf("OK\n");
}

void testRightReleaseAfterEmulate()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(20, 30), right);
  test.filterPointerEvent(rfb::Point(20, 30), both);
  test.filterPointerEvent(rfb::Point(20, 30), left); // right released

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 20);
  ASSERT_EQ(test.results[0].pos.y, 30);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 20);
  ASSERT_EQ(test.results[1].pos.y, 30);
  ASSERT_EQ(test.results[1].mask, middle);

  ASSERT_EQ(test.results[2].pos.x, 20);
  ASSERT_EQ(test.results[2].pos.y, 30);
  ASSERT_EQ(test.results[2].mask, middle);

  printf("OK\n");
}

void testLeftRepressAfterEmulate()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(20, 30), left);
  test.filterPointerEvent(rfb::Point(20, 30), both);
  test.filterPointerEvent(rfb::Point(20, 30), right); // left released
  test.filterPointerEvent(rfb::Point(20, 30), both);

  ASSERT_EQ(test.results.size(), 4);

  ASSERT_EQ(test.results[0].pos.x, 20);
  ASSERT_EQ(test.results[0].pos.y, 30);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 20);
  ASSERT_EQ(test.results[1].pos.y, 30);
  ASSERT_EQ(test.results[1].mask, middle);

  ASSERT_EQ(test.results[2].pos.x, 20);
  ASSERT_EQ(test.results[2].pos.y, 30);
  ASSERT_EQ(test.results[2].mask, middle);

  ASSERT_EQ(test.results[3].pos.x, 20);
  ASSERT_EQ(test.results[3].pos.y, 30);
  ASSERT_EQ(test.results[3].mask, middleAndLeft);

  printf("OK\n");
}

void testRightRepressAfterEmulate()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(20, 30), right);
  test.filterPointerEvent(rfb::Point(20, 30), both);
  test.filterPointerEvent(rfb::Point(20, 30), left); // right released
  test.filterPointerEvent(rfb::Point(20, 30), both);

  ASSERT_EQ(test.results.size(), 4);

  ASSERT_EQ(test.results[0].pos.x, 20);
  ASSERT_EQ(test.results[0].pos.y, 30);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 20);
  ASSERT_EQ(test.results[1].pos.y, 30);
  ASSERT_EQ(test.results[1].mask, middle);

  ASSERT_EQ(test.results[2].pos.x, 20);
  ASSERT_EQ(test.results[2].pos.y, 30);
  ASSERT_EQ(test.results[2].mask, middle);

  ASSERT_EQ(test.results[3].pos.x, 20);
  ASSERT_EQ(test.results[3].pos.y, 30);
  ASSERT_EQ(test.results[3].mask, middleAndRight);

  printf("OK\n");
}

void testBothPressAfterLeftTimeout()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(10, 20), left);
  usleep(100000); // 0.1s
  rfb::Timer::checkTimeouts();
  test.filterPointerEvent(rfb::Point(10, 20), both);

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 10);
  ASSERT_EQ(test.results[0].pos.y, 20);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 10);
  ASSERT_EQ(test.results[1].pos.y, 20);
  ASSERT_EQ(test.results[1].mask, left);

  ASSERT_EQ(test.results[2].pos.x, 10);
  ASSERT_EQ(test.results[2].pos.y, 20);
  ASSERT_EQ(test.results[2].mask, both);

  printf("OK\n");
}

void testBothPressAfterRightTimeout()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(10, 20), right);
  usleep(100000); // 0.1s
  rfb::Timer::checkTimeouts();
  test.filterPointerEvent(rfb::Point(10, 20), both);

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 10);
  ASSERT_EQ(test.results[0].pos.y, 20);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 10);
  ASSERT_EQ(test.results[1].pos.y, 20);
  ASSERT_EQ(test.results[1].mask, right);

  ASSERT_EQ(test.results[2].pos.x, 10);
  ASSERT_EQ(test.results[2].pos.y, 20);
  ASSERT_EQ(test.results[2].mask, both);

  printf("OK\n");
}

void testTimeoutAndDrag()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(0, 0), left);
  usleep(100000); //0.1s
  rfb::Timer::checkTimeouts();
  test.filterPointerEvent(rfb::Point(10, 10), left);

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 0);
  ASSERT_EQ(test.results[0].pos.y, 0);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 0);
  ASSERT_EQ(test.results[1].pos.y, 0);
  ASSERT_EQ(test.results[1].mask, left);

  ASSERT_EQ(test.results[2].pos.x, 10);
  ASSERT_EQ(test.results[2].pos.y, 10);
  ASSERT_EQ(test.results[2].mask, left);

  printf("OK\n");
}

void testDragAndTimeout()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(10, 10), left);
  test.filterPointerEvent(rfb::Point(30, 30), left);
  usleep(100000); //0.1s
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 10);
  ASSERT_EQ(test.results[0].pos.y, 10);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 10);
  ASSERT_EQ(test.results[1].pos.y, 10);
  ASSERT_EQ(test.results[1].mask, left);

  ASSERT_EQ(test.results[2].pos.x, 30);
  ASSERT_EQ(test.results[2].pos.y, 30);
  ASSERT_EQ(test.results[2].mask, left);

  printf("OK\n");
}

void testDragAndRelease()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent(rfb::Point(10, 10), left);
  test.filterPointerEvent(rfb::Point(20, 20), empty);

  ASSERT_EQ(test.results.size(), 3);

  ASSERT_EQ(test.results[0].pos.x, 10);
  ASSERT_EQ(test.results[0].pos.y, 10);
  ASSERT_EQ(test.results[0].mask, empty);

  ASSERT_EQ(test.results[1].pos.x, 10);
  ASSERT_EQ(test.results[1].pos.y, 10);
  ASSERT_EQ(test.results[1].mask, left);

  ASSERT_EQ(test.results[2].pos.x, 20);
  ASSERT_EQ(test.results[2].pos.y, 20);
  ASSERT_EQ(test.results[2].mask, empty);

  printf("OK\n");
}

int main(int argc, char** argv)
{
  testDisabledOption();

  testLeftClick();

  testNormalLeftPress();
  testNormalMiddlePress();
  testNormalRightPress();

  testEmulateMiddleMouseButton();

  testLeftReleaseAfterEmulate();
  testRightReleaseAfterEmulate();

  testLeftRepressAfterEmulate();
  testRightRepressAfterEmulate();

  testBothPressAfterLeftTimeout();
  testBothPressAfterRightTimeout();

  testTimeoutAndDrag();

  testDragAndTimeout();
  testDragAndRelease();

  return 0;
}
