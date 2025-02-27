/* Copyright 2019-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <core/string.h>

TEST(ConvertLF, convertLF)
{
  EXPECT_EQ(core::convertLF(""), "");
  EXPECT_EQ(core::convertLF("no EOL"), "no EOL");

  EXPECT_EQ(core::convertLF("\n"), "\n");
  EXPECT_EQ(core::convertLF("already correct\n"), "already correct\n");
  EXPECT_EQ(core::convertLF("multiple\nlines\n"), "multiple\nlines\n");
  EXPECT_EQ(core::convertLF("empty lines\n\n"), "empty lines\n\n");
  EXPECT_EQ(core::convertLF("\ninitial line"), "\ninitial line");

  EXPECT_EQ(core::convertLF("\r\n"), "\n");
  EXPECT_EQ(core::convertLF("one line\r\n"), "one line\n");
  EXPECT_EQ(core::convertLF("multiple\r\nlines\r\n"), "multiple\nlines\n");
  EXPECT_EQ(core::convertLF("empty lines\r\n\r\n"), "empty lines\n\n");
  EXPECT_EQ(core::convertLF("\r\ninitial line"), "\ninitial line");
  EXPECT_EQ(core::convertLF("mixed\r\nlines\n"), "mixed\nlines\n");

  EXPECT_EQ(core::convertLF("cropped\r"), "cropped\n");
  EXPECT_EQ(core::convertLF("old\rmac\rformat"), "old\nmac\nformat");
}

TEST(ConvertLF, convertCRLF)
{
  EXPECT_EQ(core::convertCRLF(""), "");
  EXPECT_EQ(core::convertCRLF("no EOL"), "no EOL");

  EXPECT_EQ(core::convertCRLF("\r\n"), "\r\n");
  EXPECT_EQ(core::convertCRLF("already correct\r\n"), "already correct\r\n");
  EXPECT_EQ(core::convertCRLF("multiple\r\nlines\r\n"), "multiple\r\nlines\r\n");
  EXPECT_EQ(core::convertCRLF("empty lines\r\n\r\n"), "empty lines\r\n\r\n");
  EXPECT_EQ(core::convertCRLF("\r\ninitial line"), "\r\ninitial line");

  EXPECT_EQ(core::convertCRLF("\n"), "\r\n");
  EXPECT_EQ(core::convertCRLF("one line\n"), "one line\r\n");
  EXPECT_EQ(core::convertCRLF("multiple\nlines\n"), "multiple\r\nlines\r\n");
  EXPECT_EQ(core::convertCRLF("empty lines\n\n"), "empty lines\r\n\r\n");
  EXPECT_EQ(core::convertCRLF("\ninitial line"), "\r\ninitial line");
  EXPECT_EQ(core::convertCRLF("mixed\r\nlines\n"), "mixed\r\nlines\r\n");

  EXPECT_EQ(core::convertCRLF("cropped\r"), "cropped\r\n");
  EXPECT_EQ(core::convertCRLF("old\rmac\rformat"), "old\r\nmac\r\nformat");
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
