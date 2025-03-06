/* Copyright 2021-2025 Pierre Ossman for Cendio AB
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

#include <gtest/gtest.h>

#define XK_LATIN1
#define XK_MISCELLANY
#include <rfb/keysymdef.h>

#include "HotKeyHandler.h"

TEST(HotKeyHandler, noCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("");

  EXPECT_EQ(handler.handleKeyPress(1, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Shift_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Control_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Hyper_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Alt_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, singleArmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);
}

TEST(HotKeyHandler, singleDualArmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);
}

TEST(HotKeyHandler, singleCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleRightCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_R), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleDualCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleComboReordered)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
}

TEST(HotKeyHandler, singleDualComboReordered)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleComboRepeated)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleComboMultipleKeys)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(3, XK_b), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(4, XK_c), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleWedgeNormal)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_b), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, singleWedgeModifier)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Shift_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, singleWedgeModifierArmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Shift_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, singleWedgeModifierFiring)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, singleUnwedge)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl");

  handler.handleKeyPress(1, XK_Shift_L);
  handler.handleKeyPress(2, XK_Control_L);
  handler.handleKeyRelease(1);
  handler.handleKeyRelease(2);

  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiArmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);
}

TEST(HotKeyHandler, multiRearmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);
}

TEST(HotKeyHandler, multiFailedArm)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, multiDualArmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_R), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyUnarm);
}

TEST(HotKeyHandler, multiCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiRightCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_R), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_R), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_R), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiDualCombo)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Alt_R), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(6, XK_Shift_R), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(7, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(7), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(6), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiComboReordered)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
}

TEST(HotKeyHandler, multiDualComboReordered)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(7, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Alt_R), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyPress(6, XK_Shift_R), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(6), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(7), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiComboRepeated)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiComboMultipleKeys)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(5, XK_b), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(6, XK_c), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(6), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiWedgeNormal)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_b), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, multiWedgeModifier)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Super_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, multiWedgeArming)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(1, XK_b), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, multiWedgeModifierArming)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Super_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, multiWedgeModifierArmed)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Super_L), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyNormal);
}

TEST(HotKeyHandler, multiWedgeModifierFiring)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Super_L), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), HotKeyHandler::KeyIgnore);
}

TEST(HotKeyHandler, multiUnwedge)
{
  HotKeyHandler handler;

  handler.setHotKeyCombo("Ctrl,Shift,Alt");

  handler.handleKeyPress(1, XK_Super_L);
  handler.handleKeyPress(2, XK_Control_L);
  handler.handleKeyPress(3, XK_Alt_L);
  handler.handleKeyPress(4, XK_Shift_L);
  handler.handleKeyRelease(1);
  handler.handleKeyRelease(2);
  handler.handleKeyRelease(3);
  handler.handleKeyRelease(4);

  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), HotKeyHandler::KeyArming);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), HotKeyHandler::KeyArm);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(5), HotKeyHandler::KeyHotKey);
  EXPECT_EQ(handler.handleKeyRelease(4), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), HotKeyHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), HotKeyHandler::KeyIgnore);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
