/* Copyright 2021 Pierre Ossman for Cendio AB
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

#define XK_LATIN1
#define XK_MISCELLANY
#include <rfb/keysymdef.h>

#include "HotKeyHandler.h"

#define ASSERT_EQ(expr, val) { \
  int _expr = (expr); \
  int _val = (val); \
  if (_expr != _val) { \
    printf("FAILED on line %d (%s equals %d, expected %d (%s))\n", __LINE__, #expr, _expr, _val, #val); \
    return; \
  } \
}

static void testNoCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("");

  ASSERT_EQ(handler.handleKeyPress(1, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Hyper_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testSingleArmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);

  printf("OK\n");
}

static void testSingleDualArmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);

  printf("OK\n");
}

static void testSingleCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleRightCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleDualCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleComboReordered()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);

  printf("OK\n");
}

static void testSingleDualComboReordered()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleComboRepeated()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleComboMultipleKeys()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(3, XK_b), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(4, XK_c), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleWedgeNormal()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_b), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testSingleWedgeModifier()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testSingleWedgeModifierArmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testSingleWedgeModifierFiring()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testSingleUnwedge()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl");

  handler.handleKeyPress(1, XK_Shift_L);
  handler.handleKeyPress(2, XK_Control_L);
  handler.handleKeyRelease(1);
  handler.handleKeyRelease(2);

  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiArmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);

  printf("OK\n");
}

static void testMultiRearmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);

  printf("OK\n");
}

static void testMultiFailedArm()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testMultiDualArmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);

  printf("OK\n");
}

static void testMultiCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiRightCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiDualCombo()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Alt_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(6, XK_Shift_R), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(7, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(7), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(6), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiComboReordered()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);

  printf("OK\n");
}

static void testMultiDualComboReordered()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(7, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Alt_R), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyPress(6, XK_Shift_R), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(6), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(7), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiComboRepeated()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiComboMultipleKeys()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(5, XK_b), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(6, XK_c), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(6), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiWedgeNormal()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_b), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testMultiWedgeModifier()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Super_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testMultiWedgeArming()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(1, XK_b), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testMultiWedgeModifierArming()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Super_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testMultiWedgeModifierArmed()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Super_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);

  printf("OK\n");
}

static void testMultiWedgeModifierFiring()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  ASSERT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyPress(5, XK_Super_L), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

static void testMultiUnwedge()
{
  HotKeyHandler handler;

  printf("%s: ", __func__);

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  handler.handleKeyPress(1, XK_Super_L);
  handler.handleKeyPress(2, XK_Control_L);
  handler.handleKeyPress(3, XK_Alt_L);
  handler.handleKeyPress(4, XK_Shift_L);
  handler.handleKeyRelease(1);
  handler.handleKeyRelease(2);
  handler.handleKeyRelease(3);
  handler.handleKeyRelease(4);

  ASSERT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  ASSERT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyHotKey);
  ASSERT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  ASSERT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);

  printf("OK\n");
}

int main(int argc, char** argv)
{
  testNoCombo();

  /* Single combo key */

  testSingleArmed();
  testSingleDualArmed();

  testSingleCombo();
  testSingleRightCombo();
  testSingleDualCombo();

  testSingleComboReordered();
  testSingleDualComboReordered();

  testSingleComboRepeated();
  testSingleComboMultipleKeys();

  testSingleWedgeNormal();
  testSingleWedgeModifier();
  testSingleWedgeModifierArmed();
  testSingleWedgeModifierFiring();

  testSingleUnwedge();

  /* Multiple combo keys */

  testMultiArmed();
  testMultiRearmed();
  testMultiFailedArm();
  testMultiDualArmed();

  testMultiCombo();
  testMultiRightCombo();
  testMultiDualCombo();

  testMultiComboReordered();
  testMultiDualComboReordered();

  testMultiComboRepeated();
  testMultiComboMultipleKeys();

  testMultiWedgeNormal();
  testMultiWedgeArming();
  testMultiWedgeModifier();
  testMultiWedgeModifierArming();
  testMultiWedgeModifierArmed();
  testMultiWedgeModifierFiring();

  testMultiUnwedge();

  return 0;
}
