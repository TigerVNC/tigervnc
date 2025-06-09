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

namespace core {

// Generic comparison for our list parameters types that preserves
// the type (doesn't use any common base class)
template<typename IterL, typename IterR,
         std::enable_if_t<std::is_member_function_pointer<decltype(&IterL::begin)>::value, bool> = true>
static bool operator==(const IterL& lhs, const IterR& rhs)
{
  if (std::distance(lhs.begin(), lhs.end()) !=
      std::distance(rhs.begin(), rhs.end()))
    return false;
  if (!std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()))
    return false;
  return true;
}

static std::ostream& operator<<(std::ostream& os,
                                const core::EnumListEntry& e)
{
  return os << '"' << e.getValueStr() << '"';
}

}

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

TEST(IntParameter, minmaxdefault)
{
  EXPECT_THROW({
    core::IntParameter defbounds("intparam", "", 10, 20, 100);
  }, std::invalid_argument);
}

TEST(IntParameter, validation)
{
  core::IntParameter valid("intparam", "", 0);

  EXPECT_TRUE(valid.setParam("123"));
  EXPECT_EQ(valid, 123);

  EXPECT_FALSE(valid.setParam("foo"));
  EXPECT_EQ(valid, 123);
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

TEST(EnumParameter, values)
{
  core::EnumParameter enums("enumparam", "", {"a", "b", "c"}, "a");

  enums.setParam("b");
  EXPECT_FALSE(enums == "a");
  EXPECT_TRUE(enums == "b");
  EXPECT_FALSE(enums == "c");
  EXPECT_FALSE(enums == "foo");
  EXPECT_FALSE(enums != "b");
  EXPECT_TRUE(enums != "c");

  enums.setParam("c");
  EXPECT_FALSE(enums == "a");
  EXPECT_FALSE(enums == "b");
  EXPECT_TRUE(enums == "c");
  EXPECT_FALSE(enums == "foo");
  EXPECT_TRUE(enums != "b");
  EXPECT_FALSE(enums != "c");
}

TEST(EnumParameter, caseinsensitive)
{
  core::EnumParameter casecmp("enumparam", "", {"a", "b", "c"}, "a");

  casecmp.setParam("B");
  EXPECT_FALSE(casecmp == "a");
  EXPECT_TRUE(casecmp == "b");
  EXPECT_TRUE(casecmp == "B");
  EXPECT_FALSE(casecmp == "c");
  EXPECT_FALSE(casecmp != "b");
  EXPECT_TRUE(casecmp != "c");
}

TEST(EnumParameter, validation)
{
  core::EnumParameter valid("enumparam", "", {"a", "b", "c"}, "a");

  EXPECT_TRUE(valid.setParam("b"));
  EXPECT_EQ(valid.getValueStr(), "b");
  EXPECT_FALSE(valid.setParam("foo"));
  EXPECT_EQ(valid.getValueStr(), "b");

  // Valid default value
  EXPECT_THROW({
    core::EnumParameter defvalid("enumparam", "", {"a", "b", "c"}, "d");
  }, std::invalid_argument);
}

TEST(EnumParameter, null)
{
  // NULL value
  core::EnumParameter null("enumparam", "", {"a", "b", "c"}, "a");
  EXPECT_THROW({
    null.setParam(nullptr);
  }, std::invalid_argument);

  // NULL default value
  EXPECT_THROW({
    core::EnumParameter defnull("enumparam", "", {""}, nullptr);
  }, std::invalid_argument);

  // NULL enum value
  EXPECT_THROW({
    core::EnumParameter nullenum("enumparam", "", {"a", nullptr, "b"}, "a");
  }, std::invalid_argument);
}

TEST(EnumParameter, encoding)
{
  core::EnumParameter encoding("enumparam", "", {"a", "b", "c"}, "a");

  encoding.setParam("b");
  EXPECT_EQ(encoding.getValueStr(), "b");

  encoding.setParam("C");
  EXPECT_EQ(encoding.getValueStr(), "c");
}

TEST(EnumParameter, default)
{
  core::EnumParameter def("enumparam", "", {"a", "b", "c"}, "a");

  EXPECT_TRUE(def.isDefault());

  def.setParam("b");
  EXPECT_FALSE(def.isDefault());

  def.setParam("A");
  EXPECT_TRUE(def.isDefault());
}

TEST(EnumParameter, immutable)
{
  core::EnumParameter immutable("enumparam", "", {"a", "b", "c"}, "a");

  immutable.setImmutable();
  immutable.setParam("b");
  immutable.setParam("c");
  EXPECT_EQ(immutable.getValueStr(), "a");
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

TEST(IntListParameter, values)
{
  std::list<int> data;

  core::IntListParameter list("listparam", "", {});

  list.setParam({1, 2, 3, 4});
  data = {1, 2, 3, 4};
  EXPECT_EQ(list, data);
}

TEST(IntListParameter, strings)
{
  std::list<int> data;

  core::IntListParameter strings("listparam", "", {});

  strings.setParam("1,2,3,4");
  data = {1, 2, 3, 4};
  EXPECT_EQ(strings, data);

  strings.setParam("5 ,   6 , 7,8");
  data = {5, 6, 7, 8};
  EXPECT_EQ(strings, data);

  strings.setParam("9,\n10,\t11,\t12");
  data = {9, 10, 11, 12};
  EXPECT_EQ(strings, data);

  strings.setParam("");
  data = {};
  EXPECT_EQ(strings, data);

  strings.setParam("   ");
  data = {};
  EXPECT_EQ(strings, data);
}

TEST(IntListParameter, minmax)
{
  std::list<int> data;

  core::IntListParameter bounds("listparam", "", {}, 20, 100);

  EXPECT_TRUE(bounds.setParam({57, 73}));
  data = {57, 73};
  EXPECT_EQ(bounds, data);

  EXPECT_FALSE(bounds.setParam({57, 123}));
  data = {57, 73};
  EXPECT_EQ(bounds, data);

  EXPECT_FALSE(bounds.setParam("57,123"));
  data = {57, 73};
  EXPECT_EQ(bounds, data);

  EXPECT_FALSE(bounds.setParam({57, -30}));
  data = {57, 73};
  EXPECT_EQ(bounds, data);

  EXPECT_FALSE(bounds.setParam("57,-30"));
  data = {57, 73};
  EXPECT_EQ(bounds, data);
}

TEST(IntListParameter, minmaxdefault)
{
  EXPECT_THROW({
    core::IntListParameter defbounds("listparam", "", {10}, 20, 100);
  }, std::invalid_argument);
}

TEST(IntListParameter, validation)
{
  std::list<int> data;

  core::IntListParameter valid("listparam", "", {});

  EXPECT_TRUE(valid.setParam("1,2,3,4"));
  data = {1, 2, 3, 4};
  EXPECT_EQ(valid, data);

  EXPECT_FALSE(valid.setParam("foo"));
  data = {1, 2, 3, 4};
  EXPECT_EQ(valid, data);

  EXPECT_FALSE(valid.setParam("1,2,x,4"));
  data = {1, 2, 3, 4};
  EXPECT_EQ(valid, data);
}

TEST(IntListParameter, encoding)
{
  std::list<int> data;

  core::IntListParameter encoding("listparam", "", {});

  encoding.setParam({1, 2, 3, 4});
  EXPECT_EQ(encoding.getValueStr(), "1,2,3,4");
}

TEST(IntListParameter, default)
{
  std::list<int> data;

  core::IntListParameter def("listparam", "", {1, 2, 3});

  EXPECT_TRUE(def.isDefault());

  def.setParam({4, 5, 6});
  EXPECT_FALSE(def.isDefault());

  def.setParam({1, 2, 3});
  EXPECT_TRUE(def.isDefault());
}

TEST(IntListParameter, immutable)
{
  std::list<int> data;

  core::IntListParameter immutable("listparam", "", {});

  immutable.setImmutable();
  immutable.setParam({1, 2, 3, 4});
  immutable.setParam("1,2,3,4");
  EXPECT_TRUE(immutable.begin() == immutable.end());
}

TEST(StringListParameter, values)
{
  std::list<std::string> data;

  core::StringListParameter list("listparam", "", {});

  list.setParam({"1", "2", "3", "4"});
  data = {"1", "2", "3", "4"};
  EXPECT_EQ(list, data);
}

TEST(StringListParameter, strings)
{
  std::list<std::string> data;

  core::StringListParameter strings("listparam", "", {});

  strings.setParam("1,2,3,4");
  data = {"1", "2", "3", "4"};
  EXPECT_EQ(strings, data);

  strings.setParam("5 ,   6 , 7,8");
  data = {"5", "6", "7", "8"};
  EXPECT_EQ(strings, data);

  strings.setParam("9,\n10,\t11,\t12");
  data = {"9", "10", "11", "12"};
  EXPECT_EQ(strings, data);

  strings.setParam("");
  data = {};
  EXPECT_EQ(strings, data);

  strings.setParam("  ");
  data = {};
  EXPECT_EQ(strings, data);

  strings.setParam("a, , b");
  data = {"a", "", "b"};
  EXPECT_EQ(strings, data);
}

TEST(StringListParameter, null)
{
  // NULL default value
  EXPECT_THROW({
    core::StringListParameter defnull("enumparam", "", {nullptr});
  }, std::invalid_argument);
}

TEST(StringListParameter, encoding)
{
  core::StringListParameter encoding("listparam", "", {});

  encoding.setParam({"1", "2", "3", "4"});
  EXPECT_EQ(encoding.getValueStr(), "1,2,3,4");
}

TEST(StringListParameter, default)
{
  core::StringListParameter def("listparam", "", {"1", "2", "3"});

  EXPECT_TRUE(def.isDefault());

  def.setParam({"4", "5", "6"});
  EXPECT_FALSE(def.isDefault());

  def.setParam({"1", "2", "3"});
  EXPECT_TRUE(def.isDefault());
}

TEST(StringListParameter, immutable)
{
  std::list<std::string> data;

  core::StringListParameter immutable("listparam", "", {"a", "b"});

  immutable.setImmutable();
  immutable.setParam({"1", "2", "3", "4"});
  immutable.setParam("1,2,3,4");
  data = {"a", "b"};
  EXPECT_EQ(immutable, data);
}

TEST(EnumListParameter, values)
{
  std::list<std::string> data;

  core::EnumListParameter list("listparam", "", {"a", "b", "c"}, {"a"});

  list.setParam({"a", "b", "c"});
  data = {"a", "b", "c"};
  EXPECT_EQ(list, data);
}

TEST(EnumListParameter, caseinsensitive)
{
  std::list<std::string> data;

  core::EnumListParameter casecmp("listparam", "", {"a", "b", "c"}, {"a"});

  casecmp.setParam({"A", "B", "C"});
  data = {"a", "B", "c"};
  EXPECT_EQ(casecmp, data);
}

TEST(EnumListParameter, strings)
{
  std::list<std::string> data;

  core::EnumListParameter strings("listparam", "", {"a", "b", "c"}, {"a"});

  strings.setParam("a,b,c");
  data = {"a", "b", "c"};
  EXPECT_EQ(strings, data);

  strings.setParam("c ,   b , a,a");
  data = {"c", "b", "a", "a"};
  EXPECT_EQ(strings, data);

  strings.setParam("b,\na,\tc,\tb");
  data = {"b", "a", "c", "b"};
  EXPECT_EQ(strings, data);

  strings.setParam("");
  data = {};
  EXPECT_EQ(strings, data);

  strings.setParam("  ");
  data = {};
  EXPECT_EQ(strings, data);
}

TEST(EnumListParameter, validation)
{
  std::list<std::string> data;

  core::EnumListParameter valid("enumparam", "", {"a", "b", "c"}, {"a"});

  EXPECT_TRUE(valid.setParam({"a", "b", "c"}));
  data = {"a", "b", "c"};
  EXPECT_EQ(valid, data);

  EXPECT_FALSE(valid.setParam({"a", "foo", "c"}));
  data = {"a", "b", "c"};
  EXPECT_EQ(valid, data);
}

TEST(EnumListParameter, validdefault)
{
  EXPECT_THROW({
    core::EnumListParameter defvalid("enumparam", "", {"a", "b", "c"}, {"d"});
  }, std::invalid_argument);
}

TEST(EnumListParameter, null)
{
  // NULL default value
  EXPECT_THROW({
    core::EnumListParameter defnull("enumparam", "", {""}, {nullptr});
  }, std::invalid_argument);

  // NULL enum value
  EXPECT_THROW({
    core::EnumListParameter nullenum("enumparam", "", {"a", nullptr, "b"}, {"a"});
  }, std::invalid_argument);
}

TEST(EnumListParameter, encoding)
{
  core::EnumListParameter encoding("listparam", "", {"a", "b", "c"}, {"a"});

  encoding.setParam({"a", "b", "C"});
  EXPECT_EQ(encoding.getValueStr(), "a,b,c");
}

TEST(EnumListParameter, default)
{
  core::EnumListParameter def("listparam", "", {"a", "b", "c"}, {"a"});

  EXPECT_TRUE(def.isDefault());

  def.setParam({"a", "b", "c"});
  EXPECT_FALSE(def.isDefault());

  def.setParam("A");
  EXPECT_TRUE(def.isDefault());
}

TEST(EnumListParameter, immutable)
{
  std::list<std::string> data;

  core::EnumListParameter immutable("listparam", "", {"a", "b", "c"}, {"a"});

  immutable.setImmutable();
  immutable.setParam({"a", "b", "c"});
  immutable.setParam("a,b,c");
  data = {"a"};
  EXPECT_EQ(immutable, data);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
