/* Copyright 2019 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <rfb/PixelFormat.h>
#include <rfb/Exception.h>

static void doTest(bool should_fail, int b, int d, bool e, bool t,
                   int rm, int gm, int bm, int rs, int gs, int bs)
{
    rfb::PixelFormat* pf;

    printf("PixelFormat(%d, %d, %s, %s, %d, %d, %d, %d, %d, %d): ",
           b, d, e ? "true" : "false", t ? "true": "false",
           rm, gm, bm, rs, gs, bs);

    try {
        pf = new rfb::PixelFormat(b, d, e, t, rm, gm, bm, rs, gs, bs);
    } catch(rfb::Exception &e) {
        if (should_fail)
            printf("OK");
        else
            printf("FAILED");
        printf("\n");
        fflush(stdout);
        return;
    }

    delete pf;

    if (should_fail)
        printf("FAILED");
    else
        printf("OK");
    printf("\n");
    fflush(stdout);
}

static void do888Test(bool expected, int b, int d, bool e, bool t,
                   int rm, int gm, int bm, int rs, int gs, int bs)
{
    rfb::PixelFormat* pf;

    printf("PixelFormat(%d, %d, %s, %s, %d, %d, %d, %d, %d, %d): ",
           b, d, e ? "true" : "false", t ? "true": "false",
           rm, gm, bm, rs, gs, bs);

    pf = new rfb::PixelFormat(b, d, e, t, rm, gm, bm, rs, gs, bs);

    if (pf->is888() == expected)
        printf("OK");
    else
        printf("FAILED");
    printf("\n");
    fflush(stdout);

    delete pf;
}

static void sanityTests()
{
    printf("Sanity checks:\n\n");

    /* Normal true color formats */

    doTest(false, 32, 24, false, true, 255, 255, 255, 0, 8, 16);
    doTest(false, 32, 24, false, true, 255, 255, 255, 24, 16, 8);

    doTest(false, 16, 16, false, true, 15, 31, 15, 0, 5, 11);

    doTest(false, 8, 8, false, true, 3, 7, 3, 0, 2, 5);

    /* Excessive bpp */

    doTest(false, 32, 16, false, true, 15, 31, 15, 0, 5, 11);

    doTest(false, 16, 16, false, true, 15, 31, 15, 0, 5, 11);

    doTest(false, 32, 8, false, true, 3, 7, 3, 0, 2, 5);

    doTest(false, 16, 8, false, true, 3, 7, 3, 0, 2, 5);

    /* Colour map */

    doTest(false, 8, 8, false, false, 0, 0, 0, 0, 0, 0);

    /* Invalid bpp */

    doTest(true, 64, 24, false, true, 255, 255, 255, 0, 8, 16);

    doTest(true, 18, 16, false, true, 15, 31, 15, 0, 5, 11);

    doTest(true, 3, 3, false, true, 1, 1, 1, 0, 1, 2);

    /* Invalid depth */

    doTest(true, 16, 24, false, true, 15, 31, 15, 0, 5, 11);

    doTest(true, 8, 24, false, true, 3, 7, 3, 0, 2, 5);
    doTest(true, 8, 16, false, true, 3, 7, 3, 0, 2, 5);

    doTest(true, 32, 24, false, false, 0, 0, 0, 0, 0, 0);

    /* Invalid max values */

    doTest(true, 32, 24, false, true, 254, 255, 255, 0, 8, 16);
    doTest(true, 32, 24, false, true, 255, 253, 255, 0, 8, 16);
    doTest(true, 32, 24, false, true, 255, 255, 252, 0, 8, 16);

    doTest(true, 32, 24, false, true, 511, 127, 127, 0, 16, 20);
    doTest(true, 32, 24, false, true, 127, 511, 127, 0, 4, 20);
    doTest(true, 32, 24, false, true, 127, 127, 511, 0, 4, 8);

    /* Insufficient depth */

    doTest(true, 32, 16, false, true, 255, 255, 255, 0, 8, 16);

    /* Invalid shift values */

    doTest(true, 32, 24, false, true, 255, 255, 255, 25, 8, 16);
    doTest(true, 32, 24, false, true, 255, 255, 255, 0, 25, 16);
    doTest(true, 32, 24, false, true, 255, 255, 255, 0, 8, 25);

    /* Overlapping channels */

    doTest(true, 32, 24, false, true, 255, 255, 255, 0, 7, 16);
    doTest(true, 32, 24, false, true, 255, 255, 255, 0, 8, 15);
    doTest(true, 32, 24, false, true, 255, 255, 255, 0, 16, 7);

    printf("\n");
}

void is888Tests()
{
    printf("Simple format detection:\n\n");

    /* Positive cases */

    do888Test(true, 32, 24, false, true, 255, 255, 255, 0, 8, 16);
    do888Test(true, 32, 24, false, true, 255, 255, 255, 24, 16, 8);
    do888Test(true, 32, 24, false, true, 255, 255, 255, 24, 8, 0);

    /* Low depth */

    do888Test(false, 32, 16, false, true, 15, 31, 15, 0, 8, 16);
    do888Test(false, 32, 8, false, true, 3, 7, 3, 0, 8, 16);

    /* Low bpp and depth */

    do888Test(false, 16, 16, false, true, 15, 31, 15, 0, 5, 11);
    do888Test(false, 8, 8, false, true, 3, 7, 3, 0, 2, 5);

    /* Colour map */

    do888Test(false, 8, 8, false, false, 0, 0, 0, 0, 0, 0);

    /* Odd shifts */

    do888Test(false, 32, 24, false, true, 255, 255, 255, 0, 8, 18);
    do888Test(false, 32, 24, false, true, 255, 255, 255, 0, 11, 24);
    do888Test(false, 32, 24, false, true, 255, 255, 255, 4, 16, 24);

    printf("\n");
}

int main(int argc, char** argv)
{
    sanityTests();
    is888Tests();

    return 0;
}
