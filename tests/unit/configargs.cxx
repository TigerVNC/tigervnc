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

#include <core/Configuration.h>

static core::BoolParameter boolparam("boolparam", "", false);
static core::IntParameter intparam("intparam", "", 0);
static core::StringParameter strparam("strparam", "", "");

#define ASSERT_EQ_I(expr, val) if ((expr) != (val)) { \
  printf("FAILED on line %d (%s equals %d, expected %d)\n", __LINE__, #expr, (int)(expr), (int)(val)); \
  return; \
}

#define ASSERT_EQ_S(expr, val) if (strcmp((expr), (val)) != 0) { \
  printf("FAILED on line %d (%s equals %s, expected %s)\n", __LINE__, #expr, (const char*)(expr), (const char*)(val)); \
  return; \
}

static void test_args()
{
  int ret;
  std::vector<const char*> argv;

  printf("%s: ", __func__);

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // Just program name
  argv = {"prog" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 0);
  ASSERT_EQ_I(ret, 0);
  ASSERT_EQ_I(boolparam, true);
  ASSERT_EQ_I(intparam, 1);
  ASSERT_EQ_S(strparam, "test");

  // A bunch of standard arguments
  argv = {"prog", "arg1", "arg2", "arg3" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 2);
  ASSERT_EQ_I(ret, 0);
  ASSERT_EQ_I(boolparam, true);
  ASSERT_EQ_I(intparam, 1);
  ASSERT_EQ_S(strparam, "test");

  // A parameter without any dash
  argv = {"prog", "strparam", "intparam" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);
  ASSERT_EQ_I(boolparam, true);
  ASSERT_EQ_I(intparam, 1);
  ASSERT_EQ_S(strparam, "test");

  printf("OK\n");
}

static void test_none()
{
  int ret;
  std::vector<const char*> argv;

  printf("%s: ", __func__);

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // int argument
  argv = {"prog", "intparam=12", "34"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(intparam, 12);

  // string argument
  argv = {"prog", "strparam=foo", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_S(strparam, "foo");

  // empty string argument
  argv = {"prog", "strparam=", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_S(strparam, "");

  // Bad parameter
  argv = {"prog", "fooparam=123"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);

  printf("OK\n");
}

static void test_single()
{
  int ret;
  std::vector<const char*> argv;

  printf("%s: ", __func__);

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // int argument
  argv = {"prog", "-intparam", "12", "34"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 2);
  ASSERT_EQ_I(intparam, 12);

  // int argument with equals
  argv = {"prog", "-intparam=12", "34"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(intparam, 12);

  // string argument
  argv = {"prog", "-strparam", "foo", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 2);
  ASSERT_EQ_S(strparam, "foo");

  // string argument with equals
  argv = {"prog", "-strparam=foo", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_S(strparam, "foo");

  // empty string argument with equals
  argv = {"prog", "-strparam=", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_S(strparam, "");

  // Missing argument
  intparam.setParam(1);
  argv = {"prog", "-intparam"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);
  ASSERT_EQ_I(intparam, 1);

  // Bad parameter
  argv = {"prog", "-fooparam", "123"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);

  // Bad parameter with equals
  argv = {"prog", "-fooparam=123"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);

  printf("OK\n");
}

static void test_double()
{
  int ret;
  std::vector<const char*> argv;

  printf("%s: ", __func__);

  boolparam.setParam(true);
  intparam.setParam(1);
  strparam.setParam("test");

  // int argument
  argv = {"prog", "--intparam", "12", "34"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 2);
  ASSERT_EQ_I(intparam, 12);

  // int argument with equals
  argv = {"prog", "--intparam=12", "34"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(intparam, 12);

  // string argument
  argv = {"prog", "--strparam", "foo", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 2);
  ASSERT_EQ_S(strparam, "foo");

  // string argument with equals
  argv = {"prog", "--strparam=foo", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_S(strparam, "foo");

  // empty string argument with equals
  argv = {"prog", "--strparam=", "bar" };
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_S(strparam, "");

  // Missing argument
  intparam.setParam(1);
  argv = {"prog", "--intparam"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);
  ASSERT_EQ_I(intparam, 1);

  // Bad parameter
  argv = {"prog", "--fooparam", "123"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);

  // Bad parameter with equals
  argv = {"prog", "--fooparam=123"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 0);

  printf("OK\n");
}

static void test_bool()
{
  int ret;
  std::vector<const char*> argv;

  printf("%s: ", __func__);

  // solo bool (single)
  boolparam.setParam(false);
  argv = {"prog", "-boolparam"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, true);

  // solo bool (double)
  boolparam.setParam(false);
  argv = {"prog", "--boolparam"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, true);

  // bool argument (single)
  boolparam.setParam(true);
  argv = {"prog", "-boolparam", "off", "on"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 2);
  ASSERT_EQ_I(boolparam, false);

  // bool argument (double)
  boolparam.setParam(true);
  argv = {"prog", "--boolparam", "off", "on"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 2);
  ASSERT_EQ_I(boolparam, false);

  // bool argument equals (single)
  boolparam.setParam(true);
  argv = {"prog", "-boolparam=off", "on"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, false);

  // bool argument equals (double)
  boolparam.setParam(true);
  argv = {"prog", "--boolparam=off", "on"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, false);

  // empty bool argument equals (single)
  boolparam.setParam(true);
  argv = {"prog", "-boolparam=", "on"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, true);

  // empty bool argument equals (double)
  boolparam.setParam(true);
  argv = {"prog", "--boolparam=", "on"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, true);

  // bool bad argument (single)
  boolparam.setParam(false);
  argv = {"prog", "-boolparam", "foo", "off"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, true);

  // bool bad argument (double)
  boolparam.setParam(false);
  argv = {"prog", "--boolparam", "foo", "off"};
  ret = core::Configuration::handleParamArg(argv.size(),
                                            (char**)argv.data(), 1);
  ASSERT_EQ_I(ret, 1);
  ASSERT_EQ_I(boolparam, true);

  printf("OK\n");
}

int main(int /*argc*/, char** /*argv*/)
{
  printf("Configuration argument handling test\n");

  test_args();
  test_none();
  test_single();
  test_double();
  test_bool();

  return 0;
}
