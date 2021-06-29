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

#include <stdio.h>
#include <vector>
#include <map>
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

#define a_code   0x001e
#define a_sym    0x0061
#define cmd_code 0x00db
#define cmd_sym  0xffe9

rfb::BoolParameter emulateMiddleButton("dummy_name", "dummy_desc", true);
rfb::BoolParameter emulateMiddleButtonMod("dummy_name", "dummy_desc", false);

class TestClass : public EmulateMB
{
public:
  TestClass() : EmulateMB(cmd_sym) {}
  virtual void sendPointerEvent(const rfb::Point& pos, int buttonMask);
  void writeKeyEvent(rdr::U32 keySym, rdr::U32 keyCode, bool down);

  struct PointerEventParams
  {
    rfb::Point pos;
    int mask;

    //Middle Button Emulation with Modifier (ALT+Left_click) middle button emulation test code.
    rdr::U32 keySym;
    rdr::U32 keyCode;
    bool down;
  };

  void handleKeyPress(int keyCode, rdr::U32 keySym);
  void handleKeyRelease(int keyCode);

  std::vector<PointerEventParams> results;
};

void TestClass::sendPointerEvent(const rfb::Point& pos, int buttonMask)
{
  PointerEventParams params = {};
  params.pos = pos;
  params.mask = buttonMask;

  results.push_back(params);
}

void TestClass::handleKeyPress(int keyCode, rdr::U32 keySym)
{
    if (filterKeyPress(keyCode, keySym))
      return;
    writeKeyEvent(keySym, keyCode, true);
}

void TestClass::handleKeyRelease(int keyCode)
{
  if (filterKeyRelease(keyCode))
    return;

  writeKeyEvent(0, keyCode, false);
}

void TestClass::writeKeyEvent(rdr::U32 keySym, rdr::U32 keyCode, bool down)
{
  PointerEventParams params = {};
  params.keySym = keySym;
  params.keyCode = keyCode;
  params.down = down;

  results.push_back(params);
}

#define ASSERT_EQ(expr, val) if ((expr) != (val)) { \
  printf("FAILED on line %d (%s equals %d, expected %d)\n", __LINE__, #expr, (int)(expr), (int)(val)); \
  return; \
}

#define PRESS true
#define RELEASE false

#define ASSERT_KEY(DOWN, CODE, SYM) do { \
  ASSERT_EQ(test.results[i].down, DOWN); \
  ASSERT_EQ(test.results[i].keyCode, CODE); \
  ASSERT_EQ(test.results[i].keySym, SYM); \
  ASSERT_EQ(test.results[i].pos.x, 0); \
  ASSERT_EQ(test.results[i].pos.y, 0); \
  ASSERT_EQ(test.results[i].mask, 0); \
  ++i; \
} while (0)

#define ASSERT_MOUSE(X, Y, MASK) do { \
  ASSERT_EQ(test.results[i].pos.x, X); \
  ASSERT_EQ(test.results[i].pos.y, Y); \
  ASSERT_EQ(test.results[i].mask, MASK); \
  ASSERT_EQ(test.results[i].down, 0); \
  ASSERT_EQ(test.results[i].keyCode, 0); \
  ASSERT_EQ(test.results[i].keySym, 0); \
  ++i; \
} while (0)

// Test press & release of 'a', press & release cmd-a.
void testMBEMKeyPress(bool enabled)
{
  TestClass test;

  printf("%s(%s): ", __func__, enabled ? "Enabled" : "Disabled");

  emulateMiddleButton.setParam(false);
  emulateMiddleButtonMod.setParam(enabled);

  test.handleKeyPress(a_code, a_sym);
  test.handleKeyRelease(a_code);

  test.handleKeyPress(cmd_code, cmd_sym);
  test.handleKeyPress(a_code, a_sym);
  test.handleKeyRelease(a_code);
  test.handleKeyRelease(cmd_code);

  ASSERT_EQ(test.results.size(), 6);

  int i = 0;
  ASSERT_KEY(PRESS,   a_code, a_sym);
  ASSERT_KEY(RELEASE, a_code, 0);

  ASSERT_KEY(PRESS,   cmd_code, cmd_sym);
  ASSERT_KEY(PRESS,   a_code, a_sym);
  ASSERT_KEY(RELEASE, a_code, 0);
  ASSERT_KEY(RELEASE, cmd_code, 0);

  printf("OK\n");
}

// Test press + release of modifier
void testMBEMModifier(bool enabled)
{
  TestClass test;

  printf("%s(%s): ", __func__, enabled ? "Enabled" : "Disabled");

  emulateMiddleButton.setParam(false);
  emulateMiddleButtonMod.setParam(enabled);

  test.handleKeyPress(cmd_code, cmd_sym);
  test.handleKeyRelease(cmd_code);

  ASSERT_EQ(test.results.size(), 2);
  int i = 0;
  ASSERT_KEY(PRESS,   cmd_code, cmd_sym);
  ASSERT_KEY(RELEASE, cmd_code, 0);

  printf("OK\n");
}

// Test left click, cmd-left click (emulate middle), cmd-left click out of order (cmd-press, button1-press, cmd-release, button1-release)
void testMBEMClick()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(false);
  emulateMiddleButtonMod.setParam(true);

  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.filterPointerEvent(rfb::Point(0, 10), empty);

  test.handleKeyPress(cmd_code, cmd_sym);
  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.filterPointerEvent(rfb::Point(0, 10), empty);
  test.handleKeyRelease(cmd_code);

  // Out of order
  test.handleKeyPress(cmd_code, cmd_sym);
  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.handleKeyRelease(cmd_code);
  test.filterPointerEvent(rfb::Point(0, 10), empty);

  ASSERT_EQ(test.results.size(), 6);
  int i = 0;
  ASSERT_MOUSE(0, 10, left);
  ASSERT_MOUSE(0, 10, empty);

  ASSERT_MOUSE(0, 10, middle);
  ASSERT_MOUSE(0, 10, empty);

  ASSERT_MOUSE(0, 10, middle);
  ASSERT_MOUSE(0, 10, empty);

  printf("OK\n");
}

// Test cmd-press, left-click, a-click, left-click
void testMBEMKeyAndClick()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(false);
  emulateMiddleButtonMod.setParam(true);

  test.handleKeyPress(cmd_code, cmd_sym);
  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.filterPointerEvent(rfb::Point(0, 10), empty);

  test.handleKeyPress(a_code, a_sym);
  test.handleKeyRelease(a_code);

  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.filterPointerEvent(rfb::Point(0, 10), empty);

  test.handleKeyRelease(cmd_code);

  ASSERT_EQ(test.results.size(), 8);
  int i = 0;
  ASSERT_MOUSE(0, 10, middle);
  ASSERT_MOUSE(0, 10, empty);

  ASSERT_KEY(PRESS,   cmd_code, cmd_sym);
  ASSERT_KEY(PRESS,   a_code, a_sym);
  ASSERT_KEY(RELEASE, a_code, 0);

  ASSERT_MOUSE(0, 10, middle);
  ASSERT_MOUSE(0, 10, empty);

  ASSERT_KEY(RELEASE, cmd_code, 0);

  printf("OK\n");
}

// Test modifier-press, button-3, modifier release.
void testMBEMRightClick()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(false);
  emulateMiddleButtonMod.setParam(true);

  test.handleKeyPress(cmd_code, cmd_sym);
  test.filterPointerEvent(rfb::Point(0, 10), right);
  test.filterPointerEvent(rfb::Point(0, 10), empty);
  test.handleKeyRelease(cmd_code);

  ASSERT_EQ(test.results.size(), 4);
  int i = 0;

  ASSERT_KEY(PRESS,   cmd_code, cmd_sym);
  ASSERT_MOUSE(0, 10, right);
  ASSERT_MOUSE(0, 10, empty);
  ASSERT_KEY(RELEASE, cmd_code, 0);

  printf("OK\n");
}

void testMBEMMultiClick()
{
  TestClass test;

  printf("%s: ", __func__);

  emulateMiddleButton.setParam(false);
  emulateMiddleButtonMod.setParam(true);

  test.handleKeyPress(cmd_code, cmd_sym);
  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.filterPointerEvent(rfb::Point(0, 10), both);
  test.filterPointerEvent(rfb::Point(0, 10), left);
  test.filterPointerEvent(rfb::Point(0, 10), empty);
  test.handleKeyRelease(cmd_code);

  ASSERT_EQ(test.results.size(), 4);
  int i = 0;
  ASSERT_MOUSE(0, 10, middle);
  ASSERT_MOUSE(0, 10, middleAndRight);
  ASSERT_MOUSE(0, 10, middle);
  ASSERT_MOUSE(0, 10, empty);

  printf("OK\n");
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

  testMBEMKeyPress(false);
  testMBEMKeyPress(true);

  testMBEMModifier(false);
  testMBEMModifier(true);

  testMBEMClick();
  testMBEMKeyAndClick();
  testMBEMRightClick();
  testMBEMMultiClick();

  return 0;
}
