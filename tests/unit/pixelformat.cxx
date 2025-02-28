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

#include <list>
#include <stdexcept>

#include <gtest/gtest.h>

#include <rfb/PixelFormat.h>

struct Params {
  int b;
  int d;
  bool e;
  bool t;
  int rm;
  int gm;
  int bm;
  int rs;
  int gs;
  int bs;
};

static std::ostream& operator<<(std::ostream& os, const Params& p)
{
  return os << p.b << ", " << p.d << ", " <<
         (p.e ? "true" : "false") << " " <<
         (p.t ? "true" : "false") << ", " <<
         p.rm << ", " << p.gm << ", " << p.bm << ", "
         << p.rs << ", " << p.gs << ", " << p.bs;
}

typedef testing::TestWithParam<Params> PixelFormatValid;

TEST_P(PixelFormatValid, constructor)
{
  Params params;
  rfb::PixelFormat* pf;

  params = GetParam();
  pf = nullptr;

  EXPECT_NO_THROW({
    pf = new rfb::PixelFormat(params.b, params.d, params.e, params.t,
                              params.rm, params.gm, params.bm,
                              params.rs, params.gs, params.bs);
  });

  delete pf;
}

typedef testing::TestWithParam<Params> PixelFormatInvalid;

TEST_P(PixelFormatInvalid, constructor)
{
  Params params;
  rfb::PixelFormat* pf;

  params = GetParam();
  pf = nullptr;

  EXPECT_THROW({
    pf = new rfb::PixelFormat(params.b, params.d, params.e, params.t,
                              params.rm, params.gm, params.bm,
                              params.rs, params.gs, params.bs);
  }, std::invalid_argument);

  delete pf;
}

typedef testing::TestWithParam<Params> PixelFormatIs888;

TEST_P(PixelFormatIs888, constructor)
{
  Params params;
  rfb::PixelFormat* pf;

  params = GetParam();
  pf = new rfb::PixelFormat(params.b, params.d, params.e, params.t,
                            params.rm, params.gm, params.bm,
                            params.rs, params.gs, params.bs);
  EXPECT_TRUE(pf->is888());

  delete pf;
}

typedef testing::TestWithParam<Params> PixelFormatNot888;

TEST_P(PixelFormatNot888, constructor)
{
  Params params;
  rfb::PixelFormat* pf;

  params = GetParam();
  pf = new rfb::PixelFormat(params.b, params.d, params.e, params.t,
                            params.rm, params.gm, params.bm,
                            params.rs, params.gs, params.bs);
  EXPECT_FALSE(pf->is888());

  delete pf;
}

static std::list<Params> validFormats()
{
  std::list<Params> params;

  /* Normal true color formats */

  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 8, 16});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 24, 16, 8});

  params.push_back(Params{16, 16, false, true, 15, 31, 15, 0, 5, 11});

  params.push_back(Params{8, 8, false, true, 3, 7, 3, 0, 2, 5});

  /* Excessive bpp */

  params.push_back(Params{32, 16, false, true, 15, 31, 15, 0, 5, 11});

  params.push_back(Params{16, 16, false, true, 15, 31, 15, 0, 5, 11});

  params.push_back(Params{32, 8, false, true, 3, 7, 3, 0, 2, 5});

  params.push_back(Params{16, 8, false, true, 3, 7, 3, 0, 2, 5});

  /* Colour map */

  params.push_back(Params{8, 8, false, false, 0, 0, 0, 0, 0, 0});

  return params;
}

INSTANTIATE_TEST_SUITE_P(, PixelFormatValid, testing::ValuesIn(validFormats()));

static std::list<Params> invalidFormats()
{
  std::list<Params> params;

  params.push_back(Params{64, 24, false, true, 255, 255, 255, 0, 8, 16});

  params.push_back(Params{18, 16, false, true, 15, 31, 15, 0, 5, 11});

  params.push_back(Params{3, 3, false, true, 1, 1, 1, 0, 1, 2});

  /* Invalid depth */

  params.push_back(Params{16, 24, false, true, 15, 31, 15, 0, 5, 11});

  params.push_back(Params{8, 24, false, true, 3, 7, 3, 0, 2, 5});
  params.push_back(Params{8, 16, false, true, 3, 7, 3, 0, 2, 5});

  params.push_back(Params{32, 24, false, false, 0, 0, 0, 0, 0, 0});

  /* Invalid max values */

  params.push_back(Params{32, 24, false, true, 254, 255, 255, 0, 8, 16});
  params.push_back(Params{32, 24, false, true, 255, 253, 255, 0, 8, 16});
  params.push_back(Params{32, 24, false, true, 255, 255, 252, 0, 8, 16});

  params.push_back(Params{32, 24, false, true, 511, 127, 127, 0, 16, 20});
  params.push_back(Params{32, 24, false, true, 127, 511, 127, 0, 4, 20});
  params.push_back(Params{32, 24, false, true, 127, 127, 511, 0, 4, 8});

  /* Insufficient depth */

  params.push_back(Params{32, 16, false, true, 255, 255, 255, 0, 8, 16});

  /* Invalid shift values */

  params.push_back(Params{32, 24, false, true, 255, 255, 255, 25, 8, 16});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 25, 16});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 8, 25});

  /* Overlapping channels */

  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 7, 16});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 8, 15});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 16, 7});

  return params;
}

INSTANTIATE_TEST_SUITE_P(, PixelFormatInvalid, testing::ValuesIn(invalidFormats()));

static std::list<Params> is888Formats()
{
  std::list<Params> params;

  /* Positive cases */

  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 8, 16});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 24, 16, 8});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 24, 8, 0});

  return params;
}

INSTANTIATE_TEST_SUITE_P(, PixelFormatIs888, testing::ValuesIn(is888Formats()));

static std::list<Params> not888Formats()
{
  std::list<Params> params;

  /* Low depth */

  params.push_back(Params{32, 16, false, true, 15, 31, 15, 0, 8, 16});
  params.push_back(Params{32, 8, false, true, 3, 7, 3, 0, 8, 16});

  /* Low bpp and depth */

  params.push_back(Params{16, 16, false, true, 15, 31, 15, 0, 5, 11});
  params.push_back(Params{8, 8, false, true, 3, 7, 3, 0, 2, 5});

  /* Colour map */

  params.push_back(Params{8, 8, false, false, 0, 0, 0, 0, 0, 0});

  /* Odd shifts */

  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 8, 18});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 0, 11, 24});
  params.push_back(Params{32, 24, false, true, 255, 255, 255, 4, 16, 24});

  return params;
}

INSTANTIATE_TEST_SUITE_P(, PixelFormatNot888, testing::ValuesIn(not888Formats()));

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
