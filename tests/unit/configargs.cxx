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

#include <gtest/gtest.h>

#include <rfb/Configuration.h>

static rfb::BoolParameter boolparam("boolparam", "", false);
static rfb::IntParameter intparam("intparam", "", 0);
static rfb::StringParameter strparam("strparam", "", "");

TEST(ConfigArgs, args)
{
  int ret;
  std::vector<const char*> argv;

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // Just program name
  argv = {"prog" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 0);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(boolparam, true);
  EXPECT_EQ(intparam, 1);
  EXPECT_STREQ(strparam, "test");

  // A bunch of standard arguments
  argv = {"prog", "arg1", "arg2", "arg3" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 2);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(boolparam, true);
  EXPECT_EQ(intparam, 1);
  EXPECT_STREQ(strparam, "test");

  // A parameter without any dash
  argv = {"prog", "strparam", "intparam" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(boolparam, true);
  EXPECT_EQ(intparam, 1);
  EXPECT_STREQ(strparam, "test");
}

TEST(ConfigArgs, noDash)
{
  int ret;
  std::vector<const char*> argv;

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // int argument
  argv = {"prog", "intparam=12", "34"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(intparam, 12);

  // string argument
  argv = {"prog", "strparam=foo", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_STREQ(strparam, "foo");

  // empty string argument
  argv = {"prog", "strparam=", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_STREQ(strparam, "");

  // Bad parameter
  argv = {"prog", "fooparam=123"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);
}

TEST(ConfigArgs, singleDash)
{
  int ret;
  std::vector<const char*> argv;

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // int argument
  argv = {"prog", "-intparam", "12", "34"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(intparam, 12);

  // int argument with equals
  argv = {"prog", "-intparam=12", "34"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(intparam, 12);

  // string argument
  argv = {"prog", "-strparam", "foo", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 2);
  EXPECT_STREQ(strparam, "foo");

  // string argument with equals
  argv = {"prog", "-strparam=foo", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_STREQ(strparam, "foo");

  // empty string argument with equals
  argv = {"prog", "-strparam=", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_STREQ(strparam, "");

  // Missing argument
  intparam.setParam(1);
  argv = {"prog", "-intparam"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(intparam, 1);

  // Bad parameter
  argv = {"prog", "-fooparam", "123"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);

  // Bad parameter with equals
  argv = {"prog", "-fooparam=123"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);
}

TEST(ConfigArgs, doubleDash)
{
  int ret;
  std::vector<const char*> argv;

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // int argument
  argv = {"prog", "--intparam", "12", "34"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(intparam, 12);

  // int argument with equals
  argv = {"prog", "--intparam=12", "34"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(intparam, 12);

  // string argument
  argv = {"prog", "--strparam", "foo", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 2);
  EXPECT_STREQ(strparam, "foo");

  // string argument with equals
  argv = {"prog", "--strparam=foo", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_STREQ(strparam, "foo");

  // empty string argument with equals
  argv = {"prog", "--strparam=", "bar" };
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_STREQ(strparam, "");

  // Missing argument
  intparam.setParam(1);
  argv = {"prog", "--intparam"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);
  EXPECT_EQ(intparam, 1);

  // Bad parameter
  argv = {"prog", "--fooparam", "123"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);

  // Bad parameter with equals
  argv = {"prog", "--fooparam=123"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 0);
}

TEST(ConfigArgs, bool)
{
  int ret;
  std::vector<const char*> argv;

  // solo bool (single)
  boolparam.setParam(false);
  argv = {"prog", "-boolparam"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, true);

  // solo bool (double)
  boolparam.setParam(false);
  argv = {"prog", "--boolparam"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, true);

  // bool argument (single)
  boolparam.setParam(true);
  argv = {"prog", "-boolparam", "off", "on"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(boolparam, false);

  // bool argument (double)
  boolparam.setParam(true);
  argv = {"prog", "--boolparam", "off", "on"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 2);
  EXPECT_EQ(boolparam, false);

  // bool argument equals (single)
  boolparam.setParam(true);
  argv = {"prog", "-boolparam=off", "on"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, false);

  // bool argument equals (double)
  boolparam.setParam(true);
  argv = {"prog", "--boolparam=off", "on"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, false);

  // empty bool argument equals (single)
  boolparam.setParam(true);
  argv = {"prog", "-boolparam=", "on"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, true);

  // empty bool argument equals (double)
  boolparam.setParam(true);
  argv = {"prog", "--boolparam=", "on"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, true);

  // bool bad argument (single)
  boolparam.setParam(false);
  argv = {"prog", "-boolparam", "foo", "off"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, true);

  // bool bad argument (double)
  boolparam.setParam(false);
  argv = {"prog", "--boolparam", "foo", "off"};
  ret = rfb::Configuration::handleParamArg(argv.size(),
                                           (char**)argv.data(), 1);
  EXPECT_EQ(ret, 1);
  EXPECT_EQ(boolparam, true);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
