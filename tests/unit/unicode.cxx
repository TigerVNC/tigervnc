/* Copyright 2020-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <string.h>
#include <wchar.h>

#include <gtest/gtest.h>

#include <core/string.h>

struct _ucs4utf8 {
    unsigned ucs4;
    const char *utf8;
};

struct _ucs4utf16 {
    unsigned ucs4;
    const wchar_t *utf16;
};

struct _latin1utf8 {
    const char *latin1;
    const char *utf8;
};

struct _utf8utf16 {
    const char *utf8;
    const wchar_t *utf16;
};

struct _ucs4utf8 ucs4utf8[] = {
    { 0x0061, "a" },
    { 0x00f6, "\xc3\xb6" },
    { 0x263a, "\xe2\x98\xba" },
    { 0x1f638, "\xf0\x9f\x98\xb8" },
    { 0x2d006, "\xf0\xad\x80\x86" },
    { 0xfffd, "\xe5\xe4" },
    { 0xfffd, "\xed\xa2\x80" },
    { 0xfffd, "\xed\xbb\xbf" },
    { 0xd880, "\xef\xbf\xbd" },
    { 0xdeff, "\xef\xbf\xbd" },
    { 0x110200, "\xef\xbf\xbd" },
};

struct _ucs4utf16 ucs4utf16[] = {
    { 0x0061, L"a" },
    { 0x00f6, L"\xf6" },
    { 0x263a, L"\x263a" },
    { 0x1f638, L"\xd83d\xde38" },
    { 0x2d006, L"\xd874\xdc06" },
    { 0xfffd, L"\xdc40\xdc12" },
    { 0x110200, L"\xfffd" },
    { 0xd87f, L"\xfffd" },
};

struct _latin1utf8 latin1utf8[] = {
    { "abc",            "abc" },
    { "\xe5\xe4\xf6",   "\xc3\xa5\xc3\xa4\xc3\xb6" },
    { "???",            "\xe2\x98\xb9\xe2\x98\xba\xe2\x98\xbb" },
    { "?",              "\xe5\xe4" },
};

struct _utf8utf16 utf8utf16[] = {
    { "abc",                                                L"abc" },
    { "\xc3\xa5\xc3\xa4\xc3\xb6",                           L"\xe5\xe4\xf6" },
    { "\xe2\x98\xb9\xe2\x98\xba\xe2\x98\xbb",               L"\x2639\x263a\x263b" },
    { "\xf0\x9f\x98\xb8\xf0\x9f\x99\x81\xf0\x9f\x99\x82",   L"\xd83d\xde38\xd83d\xde41\xd83d\xde42" },
    { "\xf0\xad\x80\x86\xf0\xad\x80\x88",                   L"\xd874\xdc06\xd874\xdc08" },
    { "\xef\xbf\xbd\xc3\xa5",                               L"\xd840\xe5" },
    { "\xed\xa1\xbf",                                       L"\xfffd" },
};

const char *validutf8[] = {
    "abc",
    "\xc3\xa5\xc3\xa4\xc3\xb6",
    "\xf0\xad\x80\x86",
};

const char *invalidutf8[] = {
    "\xe5\xe4\xf6",
    "\xf8\xa1\xa1\xa1\xa1",
    "\xed\xa2\x80",
};

const wchar_t *validutf16[] = {
    L"abc",
    L"\xe5\xe4\xf6",
    L"\xd83d\xde38\xd83d\xde41\xd83d\xde42",
};

const wchar_t *invalidutf16[] = {
    L"\xdc40\xdc12",
};

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

TEST(Unicode, ucs4ToUTF8)
{
  size_t i;
  for (i = 0;i < ARRAY_SIZE(ucs4utf8);i++) {
    size_t len;
    char utf8[5];

    /* Expected failure? */
    if (ucs4utf8[i].ucs4 == 0xfffd)
      continue;

    len = core::ucs4ToUTF8(ucs4utf8[i].ucs4, utf8);
    EXPECT_STREQ(utf8, ucs4utf8[i].utf8);
    EXPECT_EQ(len, strlen(utf8));
  }
}

TEST(Unicode, utf8ToUCS4)
{
  size_t i;
  for (i = 0;i < ARRAY_SIZE(ucs4utf8);i++) {
    size_t len;
    unsigned ucs4;

    /* Expected failure? */
    if (strcmp(ucs4utf8[i].utf8, "\xef\xbf\xbd") == 0)
      continue;

    len = core::utf8ToUCS4(ucs4utf8[i].utf8, strlen(ucs4utf8[i].utf8), &ucs4);
    EXPECT_EQ(ucs4, ucs4utf8[i].ucs4);
    EXPECT_EQ(len, strlen(ucs4utf8[i].utf8));
  }
}

TEST(Unicode, ucs4ToUTF16)
{
  size_t i;
  for (i = 0;i < ARRAY_SIZE(ucs4utf16);i++) {
    size_t len;
    wchar_t utf16[3];

    /* Expected failure? */
    if (ucs4utf16[i].ucs4 == 0xfffd)
        continue;

    len = core::ucs4ToUTF16(ucs4utf16[i].ucs4, utf16);
    EXPECT_STREQ(utf16, ucs4utf16[i].utf16);
    EXPECT_EQ(len, wcslen(utf16));
  }
}

TEST(Unicode, utf16ToUCS4)
{
  size_t i;
  for (i = 0;i < ARRAY_SIZE(ucs4utf16);i++) {
    size_t len;
    unsigned ucs4;

    /* Expected failure? */
    if (wcscmp(ucs4utf16[i].utf16, L"\xfffd") == 0)
        continue;

    len = core::utf16ToUCS4(ucs4utf16[i].utf16, wcslen(ucs4utf16[i].utf16), &ucs4);
    EXPECT_EQ(ucs4, ucs4utf16[i].ucs4);
    EXPECT_EQ(len, wcslen(ucs4utf16[i].utf16));
  }
}

TEST(Unicode, latin1ToUTF8)
{
  size_t i;

  for (i = 0; i < ARRAY_SIZE(latin1utf8); i++) {
    /* Expected failure? */
    if (strchr(latin1utf8[i].latin1, '?') != nullptr)
      continue;

    EXPECT_EQ(core::latin1ToUTF8(latin1utf8[i].latin1), latin1utf8[i].utf8);
  }
}

TEST(Unicode, utf8ToLatin1)
{
  size_t i;
  for (i = 0; i < ARRAY_SIZE(latin1utf8); i++)
    EXPECT_EQ(core::utf8ToLatin1(latin1utf8[i].utf8), latin1utf8[i].latin1);
}

TEST(Unicode, utf16ToUTF8)
{
  size_t i;
  for (i = 0; i < ARRAY_SIZE(utf8utf16); i++) {
    /* Expected failure? */
    if (wcscmp(utf8utf16[i].utf16, L"\xfffd") == 0)
      continue;

    EXPECT_EQ(core::utf16ToUTF8(utf8utf16[i].utf16), utf8utf16[i].utf8);
  }
}

TEST(Unicode, utf8ToUTF16)
{
  size_t i;
  for (i = 0; i < ARRAY_SIZE(utf8utf16); i++) {
    /* Expected failure? */
    if (strstr(utf8utf16[i].utf8, "\xef\xbf\xbd") != nullptr)
      continue;

    EXPECT_EQ(core::utf8ToUTF16(utf8utf16[i].utf8), utf8utf16[i].utf16);
  }
}

TEST(Unicode, isValidUTF8)
{
  size_t i;
  for (i = 0; i < ARRAY_SIZE(validutf8); i++)
    EXPECT_TRUE(core::isValidUTF8(validutf8[i]));
  for (i = 0; i < ARRAY_SIZE(invalidutf8); i++)
    EXPECT_FALSE(core::isValidUTF8(invalidutf8[i]));
}

TEST(Unicode, isValidUTF16)
{
  size_t i;
  for (i = 0; i < ARRAY_SIZE(validutf16); i++)
    EXPECT_TRUE(core::isValidUTF16(validutf16[i]));
  for (i = 0; i < ARRAY_SIZE(invalidutf16); i++)
    EXPECT_FALSE(core::isValidUTF16(invalidutf16[i]));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
