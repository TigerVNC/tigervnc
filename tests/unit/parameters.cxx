/* Copyright 2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <stdexcept>

#include <gtest/gtest.h>

#include <core/Configuration.h>

TEST(BoolParameter, values)
{
  core::BoolParameter bools("boolparam", "", false);

  bools.setParam(true);
  EXPECT_TRUE(bools);

  bools.setParam(false);
  EXPECT_FALSE(bools);
}

TEST(BoolParameter, strings)
{
  core::BoolParameter strings("boolparam", "", false);

  strings.setParam("on");
  EXPECT_TRUE(strings);

  strings.setParam("off");
  EXPECT_FALSE(strings);

  strings.setParam("1");
  EXPECT_TRUE(strings);

  strings.setParam("0");
  EXPECT_FALSE(strings);

  strings.setParam("true");
  EXPECT_TRUE(strings);

  strings.setParam("false");
  EXPECT_FALSE(strings);

  strings.setParam("yes");
  EXPECT_TRUE(strings);

  strings.setParam("no");
  EXPECT_FALSE(strings);
}

TEST(BoolParameter, validation)
{
  core::BoolParameter valid("boolparam", "", false);

  EXPECT_TRUE(valid.setParam("yes"));
  EXPECT_TRUE(valid);

  EXPECT_FALSE(valid.setParam("foo"));
  EXPECT_TRUE(valid);
}

TEST(BoolParameter, encoding)
{
  core::BoolParameter encoding("boolparam", "", false);

  encoding.setParam(true);
  EXPECT_EQ(encoding.getValueStr(), "on");

  encoding.setParam(false);
  EXPECT_EQ(encoding.getValueStr(), "off");
}

TEST(BoolParameter, default)
{
  core::BoolParameter def("boolparam", "", false);

  EXPECT_TRUE(def.isDefault());

  def.setParam(true);
  EXPECT_FALSE(def.isDefault());

  def.setParam(false);
  EXPECT_TRUE(def.isDefault());
}

TEST(BoolParameter, immutable)
{
  core::BoolParameter immutable("boolparam", "", false);

  immutable.setImmutable();
  immutable.setParam(true);
  immutable.setParam("on");
  EXPECT_FALSE(immutable);
}

TEST(IntParameter, values)
{
  core::IntParameter ints("intparam", "", 0);

  ints.setParam(123);
  EXPECT_EQ(ints, 123);

  ints.setParam(-456);
  EXPECT_EQ(ints, -456);
}

TEST(IntParameter, strings)
{
  core::IntParameter strings("intparam", "", 0);

  strings.setParam("123");
  EXPECT_EQ(strings, 123);

  strings.setParam("-456");
  EXPECT_EQ(strings, -456);
}

TEST(IntParameter, minmax)
{
  core::IntParameter bounds("intparam", "", 30, 20, 100);

  EXPECT_TRUE(bounds.setParam(57));
  EXPECT_EQ(bounds, 57);

  EXPECT_FALSE(bounds.setParam(123));
  EXPECT_EQ(bounds, 57);

  EXPECT_FALSE(bounds.setParam("123"));
  EXPECT_EQ(bounds, 57);

  EXPECT_FALSE(bounds.setParam(-30));
  EXPECT_EQ(bounds, 57);

  EXPECT_FALSE(bounds.setParam("-30"));
  EXPECT_EQ(bounds, 57);
}

TEST(IntParameter, encoding)
{
  core::IntParameter encoding("intparam", "", 0);

  encoding.setParam(123);
  EXPECT_EQ(encoding.getValueStr(), "123");

  encoding.setParam(-456);
  EXPECT_EQ(encoding.getValueStr(), "-456");
}

TEST(IntParameter, default)
{
  core::IntParameter def("intparam", "", 30);

  EXPECT_TRUE(def.isDefault());

  def.setParam(123);
  EXPECT_FALSE(def.isDefault());

  def.setParam(30);
  EXPECT_TRUE(def.isDefault());
}

TEST(IntParameter, immutable)
{
  core::IntParameter immutable("intparam", "", 0);

  immutable.setImmutable();
  immutable.setParam(123);
  immutable.setParam("-456");
  EXPECT_EQ(immutable, 0);
}

TEST(StringParameter, values)
{
  core::StringParameter strings("stringparam", "", "");

  strings.setParam("foo");
  EXPECT_STREQ(strings, "foo");

  strings.setParam("bar");
  EXPECT_STREQ(strings, "bar");
}

TEST(StringParameter, null)
{
  // NULL value
  core::StringParameter null("stringparam", "", "");
  EXPECT_THROW({
    null.setParam(nullptr);
  }, std::invalid_argument);

  // NULL default value
  EXPECT_THROW({
    core::StringParameter defnull("stringparam", "", nullptr);
  }, std::invalid_argument);
}

TEST(StringParameter, encoding)
{
  core::StringParameter encoding("stringparam", "", "");

  encoding.setParam("foo");
  EXPECT_EQ(encoding.getValueStr(), "foo");

  encoding.setParam("bar");
  EXPECT_EQ(encoding.getValueStr(), "bar");
}

TEST(StringParameter, default)
{
  core::StringParameter def("stringparam", "", "test");

  EXPECT_TRUE(def.isDefault());

  def.setParam("foo");
  EXPECT_FALSE(def.isDefault());

  def.setParam("test");
  EXPECT_TRUE(def.isDefault());
}

TEST(StringParameter, immutable)
{
  core::StringParameter immutable("stringparam", "", "");

  immutable.setImmutable();
  immutable.setParam("foo");
  immutable.setParam("bar");
  EXPECT_STREQ(immutable, "");
}

TEST(BinaryParameter, values)
{
  std::vector<uint8_t> data;

  core::BinaryParameter binary("binaryparam", "", nullptr, 0);

  data = {1, 2, 3};
  binary.setParam(data.data(), data.size());
  ASSERT_EQ(binary.getData().size(), 3);
  EXPECT_EQ(binary.getData()[0], 1);
  EXPECT_EQ(binary.getData()[1], 2);
  EXPECT_EQ(binary.getData()[2], 3);
}

TEST(BinaryParameter, copy)
{
  std::vector<uint8_t> data;

  core::BinaryParameter copy("binaryparam", "", nullptr, 0);

  data = {1, 2, 3};
  copy.setParam(data.data(), data.size());
  ASSERT_EQ(copy.getData().size(), 3);
  EXPECT_EQ(copy.getData()[0], 1);
  EXPECT_EQ(copy.getData()[1], 2);
  EXPECT_EQ(copy.getData()[2], 3);

  data[0] = 4;
  EXPECT_EQ(copy.getData()[0], 1);
}

TEST(BinaryParameter, strings)
{
  core::BinaryParameter strings("binaryparam", "", nullptr, 0);

  strings.setParam("010203");
  ASSERT_EQ(strings.getData().size(), 3);
  EXPECT_EQ(strings.getData()[0], 1);
  EXPECT_EQ(strings.getData()[1], 2);
  EXPECT_EQ(strings.getData()[2], 3);

  strings.setParam("deadbeef");
  ASSERT_EQ(strings.getData().size(), 4);
  EXPECT_EQ(strings.getData()[0], 0xde);
  EXPECT_EQ(strings.getData()[1], 0xad);
  EXPECT_EQ(strings.getData()[2], 0xbe);
  EXPECT_EQ(strings.getData()[3], 0xef);
}

TEST(BinaryParameter, validation)
{
  core::BinaryParameter valid("binaryparam", "", nullptr, 0);

  EXPECT_TRUE(valid.setParam("010203"));
  ASSERT_EQ(valid.getData().size(), 3);
  EXPECT_EQ(valid.getData()[0], 1);
  EXPECT_EQ(valid.getData()[1], 2);
  EXPECT_EQ(valid.getData()[2], 3);

  EXPECT_FALSE(valid.setParam("foo"));
  ASSERT_EQ(valid.getData().size(), 3);
  EXPECT_EQ(valid.getData()[0], 1);
  EXPECT_EQ(valid.getData()[1], 2);
  EXPECT_EQ(valid.getData()[2], 3);
}

TEST(BinaryParameter, encoding)
{
  std::vector<uint8_t> data;

  core::BinaryParameter encoding("binaryparam", "", nullptr, 0);

  data = {1, 2, 3};
  encoding.setParam(data.data(), data.size());
  EXPECT_EQ(encoding.getValueStr(), "010203");

  data = {0xde, 0xad, 0xbe, 0xef};
  encoding.setParam(data.data(), data.size());
  EXPECT_EQ(encoding.getValueStr(), "deadbeef");
}

TEST(BinaryParameter, default)
{
  std::vector<uint8_t> data;

  data = {1, 2, 3};
  core::BinaryParameter def("binaryparam", "", data.data(), data.size());

  EXPECT_TRUE(def.isDefault());

  data = {4, 5, 6};
  def.setParam(data.data(), data.size());
  EXPECT_FALSE(def.isDefault());

  data = {1, 2, 3};
  def.setParam(data.data(), data.size());
  EXPECT_TRUE(def.isDefault());
}

TEST(BinaryParameter, immutable)
{
  std::vector<uint8_t> data;

  core::BinaryParameter immutable("binaryparam", "", nullptr, 0);

  immutable.setImmutable();
  data = {1, 2, 3};
  immutable.setParam(data.data(), data.size());
  immutable.setParam("deadbeef");
  EXPECT_EQ(immutable.getData().size(), 0);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
