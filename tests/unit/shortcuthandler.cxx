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

#include "ShortcutHandler.h"

TEST(ShortcutHandler, noModifiers)
{
  ShortcutHandler handler;

  handler.setModifiers(0);

  EXPECT_EQ(handler.handleKeyPress(1, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Hyper_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, singleArmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyUnarm);
}

TEST(ShortcutHandler, singleDualArmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyUnarm);
}

TEST(ShortcutHandler, singleShortcut)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleRightShortcut)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleDualShortcut)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleShortcutReordered)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
}

TEST(ShortcutHandler, singleDualShortcutReordered)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleShortcutRepeated)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleShortcutMultipleKeys)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(3, XK_b), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(4, XK_c), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleWedgeNormal)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_b), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, singleWedgeModifier)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, singleWedgeModifierArmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, singleWedgeModifierFiring)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, singleUnwedge)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control);

  handler.handleKeyPress(1, XK_Shift_L);
  handler.handleKeyPress(2, XK_Control_L);
  handler.handleKeyRelease(1);
  handler.handleKeyRelease(2);

  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiArmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyUnarm);
}

TEST(ShortcutHandler, multiRearmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyUnarm);
}

TEST(ShortcutHandler, multiFailedArm)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, multiDualArmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyUnarm);
}

TEST(ShortcutHandler, multiShortcut)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiRightShortcut)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiDualShortcut)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Alt_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(6, XK_Shift_R), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(7, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(7), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(6), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiShortcutReordered)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
}

TEST(ShortcutHandler, multiDualShortcutReordered)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(7, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_R), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Alt_R), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyPress(6, XK_Shift_R), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(6), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(7), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiShortcutRepeated)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiShortcutMultipleKeys)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(5, XK_b), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(6, XK_c), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(6), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiWedgeNormal)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_b), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, multiWedgeModifier)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Super_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, multiWedgeArming)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(1, XK_b), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, multiWedgeModifierArming)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Super_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, multiWedgeModifierArmed)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Super_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyNormal);
}

TEST(ShortcutHandler, multiWedgeModifierFiring)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  EXPECT_EQ(handler.handleKeyPress(1, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(2, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyPress(5, XK_Super_L), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(1), ShortcutHandler::KeyIgnore);
}

TEST(ShortcutHandler, multiUnwedge)
{
  ShortcutHandler handler;

  handler.setModifiers(ShortcutHandler::Control |
                       ShortcutHandler::Shift |
                       ShortcutHandler::Alt);

  handler.handleKeyPress(1, XK_Super_L);
  handler.handleKeyPress(2, XK_Control_L);
  handler.handleKeyPress(3, XK_Alt_L);
  handler.handleKeyPress(4, XK_Shift_L);
  handler.handleKeyRelease(1);
  handler.handleKeyRelease(2);
  handler.handleKeyRelease(3);
  handler.handleKeyRelease(4);

  EXPECT_EQ(handler.handleKeyPress(2, XK_Control_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(3, XK_Alt_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(4, XK_Shift_L), ShortcutHandler::KeyNormal);
  EXPECT_EQ(handler.handleKeyPress(5, XK_a), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(5), ShortcutHandler::KeyShortcut);
  EXPECT_EQ(handler.handleKeyRelease(4), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(3), ShortcutHandler::KeyIgnore);
  EXPECT_EQ(handler.handleKeyRelease(2), ShortcutHandler::KeyIgnore);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
