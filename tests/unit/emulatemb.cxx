/* Copyright 2020 Alex Tanskanen <aleta@cendio.se> for Cendio AB
 * Copyright 2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <unistd.h>

#include <vector>

#include <gtest/gtest.h>

#include <core/Rect.h>
#include <core/Configuration.h>

#include "EmulateMB.h"

// The button masks for the mouse buttons
static const int empty          = 0x00;
static const int left           = 0x01;
static const int middle         = 0x02;
static const int middleAndLeft  = 0x03;
static const int right          = 0x04;
static const int both           = 0x05;
static const int middleAndRight = 0x06;

core::BoolParameter emulateMiddleButton("dummy_name", "dummy_desc", true);

class TestClass : public EmulateMB
{
public:
  void sendPointerEvent(const core::Point& pos, uint16_t buttonMask) override;

  struct PointerEventParams {core::Point pos; uint16_t mask; };

  std::vector<PointerEventParams> results;
};

void TestClass::sendPointerEvent(const core::Point& pos, uint16_t buttonMask)
{
  PointerEventParams params;
  params.pos = pos;
  params.mask = buttonMask;
  results.push_back(params);
}

TEST(EmulateMB, disabledOption)
{
  TestClass test;

  emulateMiddleButton.setParam(false);
  test.filterPointerEvent({0, 10}, left);

  ASSERT_EQ(test.results.size(), 1);

  EXPECT_EQ(test.results[0].pos.x, 0);
  EXPECT_EQ(test.results[0].pos.y, 10);
  EXPECT_EQ(test.results[0].mask, left);
}

TEST(EmulateMB, leftClick)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({0, 0}, left);
  test.filterPointerEvent({0, 0}, empty);

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 0);
  EXPECT_EQ(test.results[0].pos.y, 0);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 0);
  EXPECT_EQ(test.results[1].pos.y, 0);
  EXPECT_EQ(test.results[1].mask, left);

  EXPECT_EQ(test.results[2].pos.x, 0);
  EXPECT_EQ(test.results[2].pos.y, 0);
  EXPECT_EQ(test.results[2].mask, empty);
}

TEST(EmulateMB, normalLeftPress)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({10, 20}, left);
  usleep(100000); // 0.1s
  core::Timer::checkTimeouts();

  ASSERT_EQ(test.results.size(), 2);

  EXPECT_EQ(test.results[0].pos.x, 10);
  EXPECT_EQ(test.results[0].pos.y, 20);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 10);
  EXPECT_EQ(test.results[1].pos.y, 20);
  EXPECT_EQ(test.results[1].mask, left);
}

TEST(EmulateMB, normalMiddlePress)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({0, 0}, middle);

  ASSERT_EQ(test.results.size(), 1);

  EXPECT_EQ(test.results[0].pos.x, 0);
  EXPECT_EQ(test.results[0].pos.y, 0);
  EXPECT_EQ(test.results[0].mask, middle);
}

TEST(EmulateMB, normalRightPress)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({0, 0}, right);
  usleep(100000); // 0.1s
  core::Timer::checkTimeouts();

  ASSERT_EQ(test.results.size(), 2);

  EXPECT_EQ(test.results[0].pos.x, 0);
  EXPECT_EQ(test.results[0].pos.y, 0);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 0);
  EXPECT_EQ(test.results[1].pos.y, 0);
  EXPECT_EQ(test.results[1].mask, right);
}

TEST(EmulateMB, emulateMiddleMouseButton)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({20, 30}, right);
  test.filterPointerEvent({20, 30}, both);

  ASSERT_EQ(test.results.size(), 2);

  EXPECT_EQ(test.results[0].pos.x, 20);
  EXPECT_EQ(test.results[0].pos.y, 30);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 20);
  EXPECT_EQ(test.results[1].pos.y, 30);
  EXPECT_EQ(test.results[1].mask, middle);
}

TEST(EmulateMB, leftReleaseAfterEmulate)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({20, 30}, left);
  test.filterPointerEvent({20, 30}, both);
  test.filterPointerEvent({20, 30}, right); // left released

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 20);
  EXPECT_EQ(test.results[0].pos.y, 30);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 20);
  EXPECT_EQ(test.results[1].pos.y, 30);
  EXPECT_EQ(test.results[1].mask, middle);

  EXPECT_EQ(test.results[2].pos.x, 20);
  EXPECT_EQ(test.results[2].pos.y, 30);
  EXPECT_EQ(test.results[2].mask, middle);
}

TEST(EmulateMB, rightReleaseAfterEmulate)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({20, 30}, right);
  test.filterPointerEvent({20, 30}, both);
  test.filterPointerEvent({20, 30}, left); // right released

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 20);
  EXPECT_EQ(test.results[0].pos.y, 30);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 20);
  EXPECT_EQ(test.results[1].pos.y, 30);
  EXPECT_EQ(test.results[1].mask, middle);

  EXPECT_EQ(test.results[2].pos.x, 20);
  EXPECT_EQ(test.results[2].pos.y, 30);
  EXPECT_EQ(test.results[2].mask, middle);
}

TEST(EmulateMB, leftRepressAfterEmulate)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({20, 30}, left);
  test.filterPointerEvent({20, 30}, both);
  test.filterPointerEvent({20, 30}, right); // left released
  test.filterPointerEvent({20, 30}, both);

  ASSERT_EQ(test.results.size(), 4);

  EXPECT_EQ(test.results[0].pos.x, 20);
  EXPECT_EQ(test.results[0].pos.y, 30);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 20);
  EXPECT_EQ(test.results[1].pos.y, 30);
  EXPECT_EQ(test.results[1].mask, middle);

  EXPECT_EQ(test.results[2].pos.x, 20);
  EXPECT_EQ(test.results[2].pos.y, 30);
  EXPECT_EQ(test.results[2].mask, middle);

  EXPECT_EQ(test.results[3].pos.x, 20);
  EXPECT_EQ(test.results[3].pos.y, 30);
  EXPECT_EQ(test.results[3].mask, middleAndLeft);
}

TEST(EmulateMB, rightRepressAfterEmulate)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({20, 30}, right);
  test.filterPointerEvent({20, 30}, both);
  test.filterPointerEvent({20, 30}, left); // right released
  test.filterPointerEvent({20, 30}, both);

  ASSERT_EQ(test.results.size(), 4);

  EXPECT_EQ(test.results[0].pos.x, 20);
  EXPECT_EQ(test.results[0].pos.y, 30);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 20);
  EXPECT_EQ(test.results[1].pos.y, 30);
  EXPECT_EQ(test.results[1].mask, middle);

  EXPECT_EQ(test.results[2].pos.x, 20);
  EXPECT_EQ(test.results[2].pos.y, 30);
  EXPECT_EQ(test.results[2].mask, middle);

  EXPECT_EQ(test.results[3].pos.x, 20);
  EXPECT_EQ(test.results[3].pos.y, 30);
  EXPECT_EQ(test.results[3].mask, middleAndRight);
}

TEST(EmulateMB, bothPressAfterLeftTimeout)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({10, 20}, left);
  usleep(100000); // 0.1s
  core::Timer::checkTimeouts();
  test.filterPointerEvent({10, 20}, both);

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 10);
  EXPECT_EQ(test.results[0].pos.y, 20);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 10);
  EXPECT_EQ(test.results[1].pos.y, 20);
  EXPECT_EQ(test.results[1].mask, left);

  EXPECT_EQ(test.results[2].pos.x, 10);
  EXPECT_EQ(test.results[2].pos.y, 20);
  EXPECT_EQ(test.results[2].mask, both);
}

TEST(EmulateMB, bothPressAfterRightTimeout)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({10, 20}, right);
  usleep(100000); // 0.1s
  core::Timer::checkTimeouts();
  test.filterPointerEvent({10, 20}, both);

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 10);
  EXPECT_EQ(test.results[0].pos.y, 20);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 10);
  EXPECT_EQ(test.results[1].pos.y, 20);
  EXPECT_EQ(test.results[1].mask, right);

  EXPECT_EQ(test.results[2].pos.x, 10);
  EXPECT_EQ(test.results[2].pos.y, 20);
  EXPECT_EQ(test.results[2].mask, both);
}

TEST(EmulateMB, timeoutAndDrag)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({0, 0}, left);
  usleep(100000); //0.1s
  core::Timer::checkTimeouts();
  test.filterPointerEvent({10, 10}, left);

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 0);
  EXPECT_EQ(test.results[0].pos.y, 0);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 0);
  EXPECT_EQ(test.results[1].pos.y, 0);
  EXPECT_EQ(test.results[1].mask, left);

  EXPECT_EQ(test.results[2].pos.x, 10);
  EXPECT_EQ(test.results[2].pos.y, 10);
  EXPECT_EQ(test.results[2].mask, left);
}

TEST(EmulateMB, dragAndTimeout)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({10, 10}, left);
  test.filterPointerEvent({30, 30}, left);
  usleep(100000); //0.1s
  core::Timer::checkTimeouts();

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 10);
  EXPECT_EQ(test.results[0].pos.y, 10);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 10);
  EXPECT_EQ(test.results[1].pos.y, 10);
  EXPECT_EQ(test.results[1].mask, left);

  EXPECT_EQ(test.results[2].pos.x, 30);
  EXPECT_EQ(test.results[2].pos.y, 30);
  EXPECT_EQ(test.results[2].mask, left);
}

TEST(EmulateMB, dragAndRelease)
{
  TestClass test;

  emulateMiddleButton.setParam(true);
  test.filterPointerEvent({10, 10}, left);
  test.filterPointerEvent({20, 20}, empty);

  ASSERT_EQ(test.results.size(), 3);

  EXPECT_EQ(test.results[0].pos.x, 10);
  EXPECT_EQ(test.results[0].pos.y, 10);
  EXPECT_EQ(test.results[0].mask, empty);

  EXPECT_EQ(test.results[1].pos.x, 10);
  EXPECT_EQ(test.results[1].pos.y, 10);
  EXPECT_EQ(test.results[1].mask, left);

  EXPECT_EQ(test.results[2].pos.x, 20);
  EXPECT_EQ(test.results[2].pos.y, 20);
  EXPECT_EQ(test.results[2].mask, empty);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
