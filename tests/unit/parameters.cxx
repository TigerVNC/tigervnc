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

#include <stdio.h>
#include <string.h>

#include <stdexcept>

#include <rfb/Configuration.h>

#define ASSERT_EQ_I(expr, val) if ((expr) != (val)) { \
  printf("FAILED on line %d (%s equals %d, expected %d)\n", __LINE__, #expr, (int)(expr), (int)(val)); \
  return; \
}

#define ASSERT_EQ_S(expr, val) if (strcmp((expr), (val)) != 0) { \
  printf("FAILED on line %d (%s equals %s, expected %s)\n", __LINE__, #expr, (const char*)(expr), (const char*)(val)); \
  return; \
}

static void test_bool()
{
  printf("%s: ", __func__);

  // Boolean values
  rfb::BoolParameter bools("boolparam", "", false);
  bools.setParam(true);
  ASSERT_EQ_I(bools, true);
  bools.setParam(false);
  ASSERT_EQ_I(bools, false);

  // Boolean strings
  rfb::BoolParameter strings("boolparam", "", false);
  strings.setParam("on");
  ASSERT_EQ_I(strings, true);
  strings.setParam("off");
  ASSERT_EQ_I(strings, false);
  strings.setParam("1");
  ASSERT_EQ_I(strings, true);
  strings.setParam("0");
  ASSERT_EQ_I(strings, false);
  strings.setParam("true");
  ASSERT_EQ_I(strings, true);
  strings.setParam("false");
  ASSERT_EQ_I(strings, false);
  strings.setParam("yes");
  ASSERT_EQ_I(strings, true);
  strings.setParam("no");
  ASSERT_EQ_I(strings, false);

  // Validation
  rfb::BoolParameter valid("boolparam", "", false);
  ASSERT_EQ_I(valid.setParam("yes"), true);
  ASSERT_EQ_I(valid, true);
  ASSERT_EQ_I(valid.setParam("foo"), false);
  ASSERT_EQ_I(valid, true);

  // String encoding
  rfb::BoolParameter encoding("boolparam", "", false);
  encoding.setParam(true);
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "on");
  encoding.setParam(false);
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "off");

  // Default value
  rfb::BoolParameter def("boolparam", "", false);
  ASSERT_EQ_I(def.isDefault(), true);
  def.setParam(true);
  ASSERT_EQ_I(def.isDefault(), false);
  def.setParam(false);
  ASSERT_EQ_I(def.isDefault(), true);

  // Immutable
  rfb::BoolParameter immutable("boolparam", "", false);
  immutable.setImmutable();
  immutable.setParam(true);
  immutable.setParam("on");
  ASSERT_EQ_I(immutable, false);

  printf("OK\n");
}

static void test_int()
{
  bool ok;

  printf("%s: ", __func__);

  // Int values
  rfb::IntParameter ints("intparam", "", 0);
  ints.setParam(123);
  ASSERT_EQ_I(ints, 123);
  ints.setParam(-456);
  ASSERT_EQ_I(ints, -456);

  // Int strings
  rfb::IntParameter strings("intparam", "", 0);
  strings.setParam("123");
  ASSERT_EQ_I(strings, 123);
  strings.setParam("-456");
  ASSERT_EQ_I(strings, -456);

  // Min/Max values
  rfb::IntParameter bounds("intparam", "", 30, 20, 100);
  ASSERT_EQ_I(bounds.setParam(57), true);
  ASSERT_EQ_I(bounds, 57);
  ASSERT_EQ_I(bounds.setParam(123), false);
  ASSERT_EQ_I(bounds, 57);
  ASSERT_EQ_I(bounds.setParam("123"), false);
  ASSERT_EQ_I(bounds, 57);
  ASSERT_EQ_I(bounds.setParam(-30), false);
  ASSERT_EQ_I(bounds, 57);
  ASSERT_EQ_I(bounds.setParam("-30"), false);
  ASSERT_EQ_I(bounds, 57);

  // Min/Max default value
  try {
    rfb::IntParameter defbounds("intparam", "", 10, 20, 100);
    ok = false;
  } catch (std::exception&) {
    ok = true;
  }
  ASSERT_EQ_I(ok, true);

  // Validation
  rfb::IntParameter valid("intparam", "", 0);
  ASSERT_EQ_I(valid.setParam("123"), true);
  ASSERT_EQ_I(valid, 123);
  ASSERT_EQ_I(valid.setParam("foo"), false);
  ASSERT_EQ_I(valid, 123);

  // String encoding
  rfb::IntParameter encoding("intparam", "", 0);
  encoding.setParam(123);
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "123");
  encoding.setParam(-456);
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "-456");

  // Default value
  rfb::IntParameter def("intparam", "", 30);
  ASSERT_EQ_I(def.isDefault(), true);
  def.setParam(123);
  ASSERT_EQ_I(def.isDefault(), false);
  def.setParam(30);
  ASSERT_EQ_I(def.isDefault(), true);

  // Immutable
  rfb::IntParameter immutable("intparam", "", 0);
  immutable.setImmutable();
  immutable.setParam(123);
  immutable.setParam("-456");
  ASSERT_EQ_I(immutable, 0);

  printf("OK\n");
}

static void test_string()
{
  bool ok;

  printf("%s: ", __func__);

  // String values
  rfb::StringParameter strings("stringparam", "", "");
  strings.setParam("foo");
  ASSERT_EQ_S(strings, "foo");
  strings.setParam("bar");
  ASSERT_EQ_S(strings, "bar");

  // NULL value
  rfb::StringParameter null("stringparam", "", "");
  try {
    null.setParam(nullptr);
    ok = false;
  } catch (std::exception&) {
    ok = true;
  }
  ASSERT_EQ_I(ok, true);

  // NULL default value
  try {
    rfb::StringParameter defstrings("stringparam", "", nullptr);
    ok = false;
  } catch (std::exception&) {
    ok = true;
  }
  ASSERT_EQ_I(ok, true);

  // String encoding
  rfb::StringParameter encoding("stringparam", "", "");
  encoding.setParam("foo");
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "foo");
  encoding.setParam("bar");
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "bar");

  // Default value
  rfb::StringParameter def("stringparam", "", "test");
  ASSERT_EQ_I(def.isDefault(), true);
  def.setParam("foo");
  ASSERT_EQ_I(def.isDefault(), false);
  def.setParam("test");
  ASSERT_EQ_I(def.isDefault(), true);

  // Immutable
  rfb::StringParameter immutable("stringparam", "", "");
  immutable.setImmutable();
  immutable.setParam("foo");
  immutable.setParam("bar");
  ASSERT_EQ_S(immutable, "");

  printf("OK\n");
}

static void test_enum()
{
  bool ok;

  printf("%s: ", __func__);

  // Enum values
  rfb::EnumParameter enums("enumparam", "", {"a", "b", "c"}, "a");
  enums.setParam("b");
  ASSERT_EQ_I(enums == "a", false);
  ASSERT_EQ_I(enums == "b", true);
  ASSERT_EQ_I(enums == "c", false);
  ASSERT_EQ_I(enums == "foo", false);
  ASSERT_EQ_I(enums != "b", false);
  enums.setParam("c");
  ASSERT_EQ_I(enums == "a", false);
  ASSERT_EQ_I(enums == "b", false);
  ASSERT_EQ_I(enums == "c", true);
  ASSERT_EQ_I(enums == "foo", false);
  ASSERT_EQ_I(enums != "b", true);

  // Case insensitive
  rfb::EnumParameter casecmp("enumparam", "", {"a", "b", "c"}, "a");
  casecmp.setParam("B");
  ASSERT_EQ_I(casecmp == "a", false);
  ASSERT_EQ_I(casecmp == "b", true);
  ASSERT_EQ_I(casecmp == "B", true);
  ASSERT_EQ_I(casecmp == "c", false);
  ASSERT_EQ_I(casecmp != "b", false);

  // Validation
  rfb::EnumParameter valid("enumparam", "", {"a", "b", "c"}, "a");
  ASSERT_EQ_I(valid.setParam("b"), true);
  ASSERT_EQ_I(valid == "b", true);
  ASSERT_EQ_I(valid.setParam("foo"), false);
  ASSERT_EQ_I(valid == "b", true);

  // Valid default value
  try {
    rfb::EnumParameter defvalid("enumparam", "", {"a", "b", "c"}, "d");
    ok = false;
  } catch (std::exception&) {
    ok = true;
  }
  ASSERT_EQ_I(ok, true);

  // String encoding
  rfb::EnumParameter encoding("enumparam", "", {"a", "b", "c"}, "a");
  encoding.setParam("b");
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "b");
  encoding.setParam("C");
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "c");

  // Default value
  rfb::EnumParameter def("enumparam", "", {"a", "b", "c"}, "a");
  ASSERT_EQ_I(def.isDefault(), true);
  def.setParam("b");
  ASSERT_EQ_I(def.isDefault(), false);
  def.setParam("A");
  ASSERT_EQ_I(def.isDefault(), true);

  // Immutable
  rfb::EnumParameter immutable("enumparam", "", {"a", "b", "c"}, "a");
  immutable.setImmutable();
  immutable.setParam("b");
  immutable.setParam("c");
  ASSERT_EQ_I(immutable == "a", true);

  printf("OK\n");
}

static void test_binary()
{
  std::vector<uint8_t> data;

  printf("%s: ", __func__);

  // Binary values
  rfb::BinaryParameter binary("binaryparam", "", nullptr, 0);
  data = {1, 2, 3};
  binary.setParam(data.data(), data.size());
  ASSERT_EQ_I(binary.getData().size(), 3);
  ASSERT_EQ_I(binary.getData()[0], 1);
  ASSERT_EQ_I(binary.getData()[1], 2);
  ASSERT_EQ_I(binary.getData()[2], 3);

  // Data copy
  rfb::BinaryParameter copy("binaryparam", "", nullptr, 0);
  data = {1, 2, 3};
  copy.setParam(data.data(), data.size());
  ASSERT_EQ_I(binary.getData().size(), 3);
  ASSERT_EQ_I(binary.getData()[0], 1);
  ASSERT_EQ_I(binary.getData()[1], 2);
  ASSERT_EQ_I(binary.getData()[2], 3);
  data[0] = 4;
  ASSERT_EQ_I(binary.getData()[0], 1);

  // Binary strings
  rfb::BinaryParameter strings("binaryparam", "", nullptr, 0);
  strings.setParam("010203");
  ASSERT_EQ_I(strings.getData().size(), 3);
  ASSERT_EQ_I(strings.getData()[0], 1);
  ASSERT_EQ_I(strings.getData()[1], 2);
  ASSERT_EQ_I(strings.getData()[2], 3);
  strings.setParam("deadbeef");
  ASSERT_EQ_I(strings.getData().size(), 4);
  ASSERT_EQ_I(strings.getData()[0], 0xde);
  ASSERT_EQ_I(strings.getData()[1], 0xad);
  ASSERT_EQ_I(strings.getData()[2], 0xbe);
  ASSERT_EQ_I(strings.getData()[3], 0xef);

  // Validation
  rfb::BinaryParameter valid("binaryparam", "", nullptr, 0);
  ASSERT_EQ_I(valid.setParam("010203"), true);
  ASSERT_EQ_I(valid.getData().size(), 3);
  ASSERT_EQ_I(valid.getData()[0], 1);
  ASSERT_EQ_I(valid.getData()[1], 2);
  ASSERT_EQ_I(valid.getData()[2], 3);
  ASSERT_EQ_I(valid.setParam("foo"), false);
  ASSERT_EQ_I(valid.getData().size(), 3);
  ASSERT_EQ_I(valid.getData()[0], 1);
  ASSERT_EQ_I(valid.getData()[1], 2);
  ASSERT_EQ_I(valid.getData()[2], 3);

  // String encoding
  rfb::BinaryParameter encoding("binaryparam", "", nullptr, 0);
  data = {1, 2, 3};
  encoding.setParam(data.data(), data.size());
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "010203");
  data = {0xde, 0xad, 0xbe, 0xef};
  encoding.setParam(data.data(), data.size());
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "deadbeef");

  // Default value
  data = {1, 2, 3};
  rfb::BinaryParameter def("binaryparam", "", data.data(), data.size());
  ASSERT_EQ_I(def.isDefault(), true);
  data = {4, 5, 6};
  def.setParam(data.data(), data.size());
  ASSERT_EQ_I(def.isDefault(), false);
  data = {1, 2, 3};
  def.setParam(data.data(), data.size());
  ASSERT_EQ_I(def.isDefault(), true);

  // Immutable
  rfb::BinaryParameter immutable("binaryparam", "", nullptr, 0);
  immutable.setImmutable();
  data = {1, 2, 3};
  immutable.setParam(data.data(), data.size());
  immutable.setParam("deadbeef");
  ASSERT_EQ_I(immutable.getData().size(), 0);

  printf("OK\n");
}

static void test_int_list()
{
  bool ok;
  std::list<int> data;

  printf("%s: ", __func__);

  // List values
  rfb::IntListParameter list("listparam", "", {});
  list.setParam({1, 2, 3, 4});
  data = {1, 2, 3, 4};
  ASSERT_EQ_I(std::distance(list.begin(), list.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(list.begin(), list.end(), data.begin()), true);

  // List strings
  rfb::IntListParameter strings("listparam", "", {});
  strings.setParam("1,2,3,4");
  data = {1, 2, 3, 4};
  ASSERT_EQ_I(std::distance(strings.begin(), strings.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(strings.begin(), strings.end(), data.begin()), true);

  // Min/Max values
  rfb::IntListParameter bounds("listparam", "", {}, 20, 100);
  ASSERT_EQ_I(bounds.setParam({57, 73}), true);
  data = {57, 73};
  ASSERT_EQ_I(std::distance(bounds.begin(), bounds.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(bounds.begin(), bounds.end(), data.begin()), true);
  ASSERT_EQ_I(bounds.setParam({57, 123}), false);
  data = {57, 73};
  ASSERT_EQ_I(std::distance(bounds.begin(), bounds.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(bounds.begin(), bounds.end(), data.begin()), true);
  ASSERT_EQ_I(bounds.setParam("57,123"), false);
  data = {57, 73};
  ASSERT_EQ_I(std::distance(bounds.begin(), bounds.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(bounds.begin(), bounds.end(), data.begin()), true);
  ASSERT_EQ_I(bounds.setParam({57, -30}), false);
  data = {57, 73};
  ASSERT_EQ_I(std::distance(bounds.begin(), bounds.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(bounds.begin(), bounds.end(), data.begin()), true);
  ASSERT_EQ_I(bounds.setParam("57,-30"), false);
  data = {57, 73};
  ASSERT_EQ_I(std::distance(bounds.begin(), bounds.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(bounds.begin(), bounds.end(), data.begin()), true);

  // Min/Max default value
  try {
    rfb::IntListParameter defbounds("listparam", "", {10}, 20, 100);
    ok = false;
  } catch (std::exception&) {
    ok = true;
  }
  ASSERT_EQ_I(ok, true);

  // Validation
  rfb::IntListParameter valid("listparam", "", {});
  ASSERT_EQ_I(valid.setParam("1,2,3,4"), true);
  data = {1, 2, 3, 4};
  ASSERT_EQ_I(std::distance(valid.begin(), valid.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(valid.begin(), valid.end(), data.begin()), true);
  ASSERT_EQ_I(valid.setParam("foo"), false);
  data = {1, 2, 3, 4};
  ASSERT_EQ_I(std::distance(valid.begin(), valid.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(valid.begin(), valid.end(), data.begin()), true);
  ASSERT_EQ_I(valid.setParam("1,2,x,4"), false);
  data = {1, 2, 3, 4};
  ASSERT_EQ_I(std::distance(valid.begin(), valid.end()), std::distance(data.begin(), data.end()));
  ASSERT_EQ_I(std::equal(valid.begin(), valid.end(), data.begin()), true);

  // String encoding
  rfb::IntListParameter encoding("listparam", "", {});
  encoding.setParam({1, 2, 3, 4});
  ASSERT_EQ_S(encoding.getValueStr().c_str(), "1,2,3,4");

  // Default value
  rfb::IntListParameter def("listparam", "", {1, 2, 3});
  ASSERT_EQ_I(def.isDefault(), true);
  def.setParam({4, 5, 6});
  ASSERT_EQ_I(def.isDefault(), false);
  def.setParam({1, 2, 3});
  ASSERT_EQ_I(def.isDefault(), true);

  // Immutable
  rfb::IntListParameter immutable("listparam", "", {});
  immutable.setImmutable();
  immutable.setParam({1, 2, 3, 4});
  immutable.setParam("1,2,3,4");
  ASSERT_EQ_I(immutable.begin() == immutable.end(), true);

  printf("OK\n");
}

int main(int /*argc*/, char** /*argv*/)
{
  printf("Parameters test\n");

  test_bool();
  test_int();
  test_string();
  test_enum();
  test_binary();

  test_int_list();

  return 0;
}
