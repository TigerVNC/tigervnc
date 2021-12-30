/* Copyright 2020 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
#include <unistd.h>

#include <vector>

#include "../../vncviewer/GestureHandler.h"

class TestClass : public GestureHandler
{
  protected:
    virtual void handleGestureEvent(const GestureEvent& event);

  public:
    std::vector<GestureEvent> events;
};

void TestClass::handleGestureEvent(const GestureEvent& event)
{
  events.push_back(event);
}

// FIXME: handle doubles
#define ASSERT_EQ(expr, val) if ((expr) != (val)) { \
  printf("FAILED on line %d (%s equals %d, expected %d)\n", __LINE__, #expr, (int)(expr), (int)(val)); \
  return; \
}

void testOneTapNormal()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureOneTap);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  ASSERT_EQ(test.events[1].type, GestureEnd);
  ASSERT_EQ(test.events[1].gesture, GestureOneTap);
  ASSERT_EQ(test.events[1].eventX, 20.0);
  ASSERT_EQ(test.events[1].eventY, 30.0);

  printf("OK\n");
}

void testTwoTapNormal()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoTap);
  ASSERT_EQ(test.events[0].eventX, 25.0);
  ASSERT_EQ(test.events[0].eventY, 40.0);

  ASSERT_EQ(test.events[1].type, GestureEnd);
  ASSERT_EQ(test.events[1].gesture, GestureTwoTap);
  ASSERT_EQ(test.events[1].eventX, 25.0);
  ASSERT_EQ(test.events[1].eventY, 40.0);

  printf("OK\n");
}

void testTwoTapSlowBegin()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchBegin(2, 30.0, 50.0);
  test.handleTouchEnd(1);
  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testTwoTapSlowEnd()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);
  test.handleTouchEnd(1);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testTwoTapTimeout()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);

  usleep(1500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchEnd(1);
  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testThreeTapNormal()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);
  test.handleTouchBegin(3, 40.0, 40.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureThreeTap);
  ASSERT_EQ(test.events[0].eventX, 30.0);
  ASSERT_EQ(test.events[0].eventY, 40.0);

  ASSERT_EQ(test.events[1].type, GestureEnd);
  ASSERT_EQ(test.events[1].gesture, GestureThreeTap);
  ASSERT_EQ(test.events[1].eventX, 30.0);
  ASSERT_EQ(test.events[1].eventY, 40.0);

  printf("OK\n");
}

void testThreeTapSlowBegin()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchBegin(3, 40.0, 40.0);
  test.handleTouchEnd(1);
  test.handleTouchEnd(2);
  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testThreeTapSlowEnd()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);
  test.handleTouchBegin(3, 40.0, 40.0);
  test.handleTouchEnd(1);
  test.handleTouchEnd(2);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testThreeTapDrag()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);
  test.handleTouchBegin(3, 40.0, 40.0);

  test.handleTouchUpdate(1, 120.0, 130.0);
  test.handleTouchUpdate(2, 130.0, 150.0);
  test.handleTouchUpdate(3, 140.0, 140.0);

  test.handleTouchEnd(1);
  test.handleTouchEnd(2);
  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testThreeTapTimeout()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 50.0);
  test.handleTouchBegin(3, 40.0, 40.0);

  usleep(1500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchEnd(1);
  test.handleTouchEnd(2);
  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testDragHoriz()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 40.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 80.0, 30.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureDrag);
  ASSERT_EQ(test.events[1].eventX, 80.0);
  ASSERT_EQ(test.events[1].eventY, 30.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 80.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  printf("OK\n");
}

void testDragVert()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 20.0, 50.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 20.0, 90.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureDrag);
  ASSERT_EQ(test.events[1].eventX, 20.0);
  ASSERT_EQ(test.events[1].eventY, 90.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 90.0);

  printf("OK\n");
}

void testDragDiag()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 120.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 90.0, 100.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 60.0, 70.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 120.0);
  ASSERT_EQ(test.events[0].eventY, 130.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureDrag);
  ASSERT_EQ(test.events[1].eventX, 60.0);
  ASSERT_EQ(test.events[1].eventY, 70.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 60.0);
  ASSERT_EQ(test.events[0].eventY, 70.0);

  printf("OK\n");
}

void testLongPressNormal()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(1500000);
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureLongPress);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureLongPress);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  printf("OK\n");
}

void testLongPressDrag()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(1500000);
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureLongPress);
  ASSERT_EQ(test.events[0].eventX, 20.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);

  test.events.clear();

  test.handleTouchUpdate(1, 120.0, 50.0);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureUpdate);
  ASSERT_EQ(test.events[0].gesture, GestureLongPress);
  ASSERT_EQ(test.events[0].eventX, 120.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureLongPress);
  ASSERT_EQ(test.events[0].eventX, 120.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);

  printf("OK\n");
}

void testTwoDragFastDistinctHoriz()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 40.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 50.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 90.0, 30.0);
  test.handleTouchUpdate(1, 80.0, 30.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 25.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[1].eventX, 25.0);
  ASSERT_EQ(test.events[1].eventY, 30.0);
  ASSERT_EQ(test.events[1].magnitudeX, 60.0);
  ASSERT_EQ(test.events[1].magnitudeY, 0.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 25.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);
  ASSERT_EQ(test.events[0].magnitudeX, 60.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  printf("OK\n");
}

void testTwoDragFastDistinctVert()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 20.0, 100.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 30.0, 40.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 30.0, 90.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 25.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[1].eventX, 25.0);
  ASSERT_EQ(test.events[1].eventY, 30.0);
  ASSERT_EQ(test.events[1].magnitudeX, 0.0);
  ASSERT_EQ(test.events[1].magnitudeY, 65.0);

  test.events.clear();

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 25.0);
  ASSERT_EQ(test.events[0].eventY, 30.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 65.0);

  printf("OK\n");
}

void testTwoDragFastDistinctDiag()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 120.0, 130.0);
  test.handleTouchBegin(2, 130.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 80.0, 90.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 100.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 60.0, 70.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 125.0);
  ASSERT_EQ(test.events[0].eventY, 130.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[1].eventX, 125.0);
  ASSERT_EQ(test.events[1].eventY, 130.0);
  ASSERT_EQ(test.events[1].magnitudeX, -55.0);
  ASSERT_EQ(test.events[1].magnitudeY, -50.0);

  test.events.clear();

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 125.0);
  ASSERT_EQ(test.events[0].eventY, 130.0);
  ASSERT_EQ(test.events[0].magnitudeX, -55.0);
  ASSERT_EQ(test.events[0].magnitudeY, -50.0);

  printf("OK\n");
}

void testTwoDragFastAlmost()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 30.0);
  test.handleTouchUpdate(1, 80.0, 30.0);
  test.handleTouchUpdate(2, 70.0, 30.0);
  test.handleTouchEnd(1);
  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 0);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testTwoDragSlowHoriz()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 50.0, 40.0);
  test.handleTouchBegin(2, 60.0, 40.0);
  test.handleTouchUpdate(2, 80.0, 40.0);
  test.handleTouchUpdate(1, 110.0, 40.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(60000); // 60ms
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 2);
  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 55.0);
  ASSERT_EQ(test.events[0].eventY, 40.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[1].eventX, 55.0);
  ASSERT_EQ(test.events[1].eventY, 40.0);
  ASSERT_EQ(test.events[1].magnitudeX, 40.0);
  ASSERT_EQ(test.events[1].magnitudeY, 0.0);

  printf("OK\n");
}

void testTwoDragSlowVert()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 40.0, 40.0);
  test.handleTouchBegin(2, 40.0, 60.0);
  test.handleTouchUpdate(2, 40.0, 80.0);
  test.handleTouchUpdate(1, 40.0, 100.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(60000); // 60ms
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 2);
  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 40.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[1].eventX, 40.0);
  ASSERT_EQ(test.events[1].eventY, 50.0);
  ASSERT_EQ(test.events[1].magnitudeX, 0.0);
  ASSERT_EQ(test.events[1].magnitudeY, 40.0);

  printf("OK\n");
}

void testTwoDragSlowDiag()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 50.0, 40.0);
  test.handleTouchBegin(2, 40.0, 60.0);
  test.handleTouchUpdate(1, 70.0, 60.0);
  test.handleTouchUpdate(2, 90.0, 110.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(60000); // 60ms
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 2);
  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[0].eventX, 45.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);
  ASSERT_EQ(test.events[0].magnitudeX, 0.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);
  ASSERT_EQ(test.events[1].eventX, 45.0);
  ASSERT_EQ(test.events[1].eventY, 50.0);
  ASSERT_EQ(test.events[1].magnitudeX, 35.0);
  ASSERT_EQ(test.events[1].magnitudeY, 35.0);

  printf("OK\n");
}

void testTwoDragTooSlow()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  test.handleTouchBegin(2, 30.0, 30.0);
  test.handleTouchUpdate(2, 50.0, 30.0);
  test.handleTouchUpdate(1, 80.0, 30.0);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testPinchFastDistinctIn()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 0.0, 0.0);
  test.handleTouchBegin(2, 130.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 50.0, 40.0);
  test.handleTouchUpdate(2, 100.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 60.0, 70.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GesturePinch);
  ASSERT_EQ(test.events[0].eventX, 65.0);
  ASSERT_EQ(test.events[0].eventY, 65.0);
  ASSERT_EQ(test.events[0].magnitudeX, 130.0);
  ASSERT_EQ(test.events[0].magnitudeY, 130.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GesturePinch);
  ASSERT_EQ(test.events[1].eventX, 65.0);
  ASSERT_EQ(test.events[1].eventY, 65.0);
  ASSERT_EQ(test.events[1].magnitudeX, 10.0);
  ASSERT_EQ(test.events[1].magnitudeY, 30.0);

  test.events.clear();

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GesturePinch);
  ASSERT_EQ(test.events[0].eventX, 65.0);
  ASSERT_EQ(test.events[0].eventY, 65.0);
  ASSERT_EQ(test.events[0].magnitudeX, 10.0);
  ASSERT_EQ(test.events[0].magnitudeY, 30.0);

  printf("OK\n");
}

void testPinchFastDistinctOut()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 100.0, 100.0);
  test.handleTouchBegin(2, 110.0, 100.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 130.0, 70.0);
  test.handleTouchUpdate(2, 0.0, 200.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 180.0, 20.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GesturePinch);
  ASSERT_EQ(test.events[0].eventX, 105.0);
  ASSERT_EQ(test.events[0].eventY, 100.0);
  ASSERT_EQ(test.events[0].magnitudeX, 10.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GesturePinch);
  ASSERT_EQ(test.events[1].eventX, 105.0);
  ASSERT_EQ(test.events[1].eventY, 100.0);
  ASSERT_EQ(test.events[1].magnitudeX, 180.0);
  ASSERT_EQ(test.events[1].magnitudeY, 180.0);

  test.events.clear();

  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GesturePinch);
  ASSERT_EQ(test.events[0].eventX, 105.0);
  ASSERT_EQ(test.events[0].eventY, 100.0);
  ASSERT_EQ(test.events[0].magnitudeX, 180.0);
  ASSERT_EQ(test.events[0].magnitudeY, 180.0);

  printf("OK\n");
}

void testPinchFastAlmost()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 130.0, 130.0);
  test.handleTouchUpdate(1, 80.0, 70.0);
  test.handleTouchEnd(1);
  test.handleTouchEnd(2);

  ASSERT_EQ(test.events.size(), 0);

  usleep(500000);
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testPinchSlowIn()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 0.0, 0.0);
  test.handleTouchBegin(2, 130.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 50.0, 40.0);
  test.handleTouchUpdate(2, 100.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(60000); // 60ms
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GesturePinch);
  ASSERT_EQ(test.events[0].eventX, 65.0);
  ASSERT_EQ(test.events[0].eventY, 65.0);
  ASSERT_EQ(test.events[0].magnitudeX, 130.0);
  ASSERT_EQ(test.events[0].magnitudeY, 130.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GesturePinch);
  ASSERT_EQ(test.events[1].eventX, 65.0);
  ASSERT_EQ(test.events[1].eventY, 65.0);
  ASSERT_EQ(test.events[1].magnitudeX, 50.0);
  ASSERT_EQ(test.events[1].magnitudeY, 90.0);

  printf("OK\n");
}

void testPinchSlowOut()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 100.0, 130.0);
  test.handleTouchBegin(2, 110.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(2, 200.0, 130.0);

  ASSERT_EQ(test.events.size(), 0);

  usleep(60000); // 60ms
  rfb::Timer::checkTimeouts();

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GesturePinch);
  ASSERT_EQ(test.events[0].eventX, 105.0);
  ASSERT_EQ(test.events[0].eventY, 130.0);
  ASSERT_EQ(test.events[0].magnitudeX, 10.0);
  ASSERT_EQ(test.events[0].magnitudeY, 0.0);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GesturePinch);
  ASSERT_EQ(test.events[1].eventX, 105.0);
  ASSERT_EQ(test.events[1].eventY, 130.0);
  ASSERT_EQ(test.events[1].magnitudeX, 100.0);
  ASSERT_EQ(test.events[1].magnitudeY, 0.0);

  printf("OK\n");
}

void testPinchTooSlow()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 0.0, 0.0);

  usleep(60000); // 60ms
  rfb::Timer::checkTimeouts();

  test.handleTouchBegin(2, 130.0, 130.0);
  test.handleTouchUpdate(2, 100.0, 130.0);
  test.handleTouchUpdate(1, 50.0, 40.0);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testExtraIgnore()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchUpdate(1, 40.0, 30.0);
  test.handleTouchUpdate(1, 80.0, 30.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureDrag);

  test.events.clear();

  test.handleTouchBegin(2, 10.0, 10.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 100.0, 50.0);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureUpdate);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 100.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 100.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);

  printf("OK\n");
}

void testIgnoreWhenAwaitingGestureEnd()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchBegin(2, 30.0, 30.0);
  test.handleTouchUpdate(1, 40.0, 30.0);
  test.handleTouchUpdate(2, 90.0, 30.0);
  test.handleTouchUpdate(1, 80.0, 30.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureTwoDrag);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureTwoDrag);

  test.events.clear();

  test.handleTouchBegin(3, 10.0, 10.0);
  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 0);

  printf("OK\n");
}

void testIgnoreAfterGesture()
{
  TestClass test;

  printf("%s: ", __func__);

  test.handleTouchBegin(1, 20.0, 30.0);
  test.handleTouchUpdate(1, 40.0, 30.0);
  test.handleTouchUpdate(1, 80.0, 30.0);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);

  ASSERT_EQ(test.events[1].type, GestureUpdate);
  ASSERT_EQ(test.events[1].gesture, GestureDrag);

  test.events.clear();

  // Start ignored event
  test.handleTouchBegin(2, 10.0, 10.0);

  ASSERT_EQ(test.events.size(), 0);

  test.handleTouchUpdate(1, 100.0, 50.0);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureUpdate);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 100.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);

  test.events.clear();

  test.handleTouchEnd(1);

  ASSERT_EQ(test.events.size(), 1);

  ASSERT_EQ(test.events[0].type, GestureEnd);
  ASSERT_EQ(test.events[0].gesture, GestureDrag);
  ASSERT_EQ(test.events[0].eventX, 100.0);
  ASSERT_EQ(test.events[0].eventY, 50.0);

  // End ignored event
  test.handleTouchEnd(2);

  // Check that everything is reseted after trailing ignores are released
  test.events.clear();

  test.handleTouchBegin(3, 20.0, 30.0);
  test.handleTouchEnd(3);

  ASSERT_EQ(test.events.size(), 2);

  ASSERT_EQ(test.events[0].type, GestureBegin);
  ASSERT_EQ(test.events[0].gesture, GestureOneTap);
  ASSERT_EQ(test.events[1].type, GestureEnd);
  ASSERT_EQ(test.events[1].gesture, GestureOneTap);

  printf("OK\n");
}

void testOneTap()
{
  testOneTapNormal();
}

void testTwoTap()
{
  testTwoTapNormal();
  testTwoTapSlowBegin();
  testTwoTapSlowEnd();
  testTwoTapTimeout();
}

void testThreeTap()
{
  testThreeTapNormal();
  testThreeTapSlowBegin();
  testThreeTapSlowEnd();
  testThreeTapDrag();
  testThreeTapTimeout();
}

void testDrag()
{
  testDragHoriz();
  testDragVert();
  testDragDiag();
}

void testLongPress()
{
  testLongPressNormal();
  testLongPressDrag();
}

void testTwoDrag()
{
  testTwoDragFastDistinctHoriz();
  testTwoDragFastDistinctVert();
  testTwoDragFastDistinctDiag();
  testTwoDragFastAlmost();
  testTwoDragSlowHoriz();
  testTwoDragSlowVert();
  testTwoDragSlowDiag();
  testTwoDragTooSlow();
}

void testPinch()
{
  testPinchFastDistinctIn();
  testPinchFastDistinctOut();
  testPinchFastAlmost();
  testPinchSlowIn();
  testPinchSlowOut();
  testPinchTooSlow();
}

void testIgnore()
{
  testExtraIgnore();
  testIgnoreWhenAwaitingGestureEnd();
  testIgnoreAfterGesture();
}

int main(int argc, char** argv)
{
  testOneTap();
  testTwoTap();
  testThreeTap();
  testDrag();
  testLongPress();
  testTwoDrag();
  testPinch();
  testIgnore();

  return 0;
}
