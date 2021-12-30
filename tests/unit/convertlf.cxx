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

#include <rfb/util.h>

static const char* escape(const char* input)
{
    static char output[4096];

    const char* in;
    char* out;

    in = input;
    out = output;
    do {
        if (*in == '\r') {
            *out++ = '\\';
            *out++ = 'r';
        } else if (*in == '\n') {
            *out++ = '\\';
            *out++ = 'n';
        } else {
            *out++ = *in;
        }
    } while (*in++ != '\0');

    return output;
}

static void testLF(const char* input, const char* expected)
{
    char* output;

    printf("convertLF(\"%s\"): ", escape(input));

    output = rfb::convertLF(input);

    if (strcmp(output, expected) != 0)
        printf("FAILED: got \"%s\"", escape(output));
    else
        printf("OK");
    printf("\n");
    fflush(stdout);

    rfb::strFree(output);
}

static void testCRLF(const char* input, const char* expected)
{
    char* output;

    printf("convertCRLF(\"%s\"): ", escape(input));

    output = rfb::convertCRLF(input);

    if (strcmp(output, expected) != 0)
        printf("FAILED: got \"%s\"", escape(output));
    else
        printf("OK");
    printf("\n");
    fflush(stdout);

    rfb::strFree(output);
}

int main(int argc, char** argv)
{
    testLF("", "");
    testLF("no EOL", "no EOL");

    testLF("\n", "\n");
    testLF("already correct\n", "already correct\n");
    testLF("multiple\nlines\n", "multiple\nlines\n");
    testLF("empty lines\n\n", "empty lines\n\n");
    testLF("\ninitial line", "\ninitial line");

    testLF("\r\n", "\n");
    testLF("one line\r\n", "one line\n");
    testLF("multiple\r\nlines\r\n", "multiple\nlines\n");
    testLF("empty lines\r\n\r\n", "empty lines\n\n");
    testLF("\r\ninitial line", "\ninitial line");
    testLF("mixed\r\nlines\n", "mixed\nlines\n");

    testLF("cropped\r", "cropped\n");
    testLF("old\rmac\rformat", "old\nmac\nformat");

    testCRLF("", "");
    testCRLF("no EOL", "no EOL");

    testCRLF("\r\n", "\r\n");
    testCRLF("already correct\r\n", "already correct\r\n");
    testCRLF("multiple\r\nlines\r\n", "multiple\r\nlines\r\n");
    testCRLF("empty lines\r\n\r\n", "empty lines\r\n\r\n");
    testCRLF("\r\ninitial line", "\r\ninitial line");

    testCRLF("\n", "\r\n");
    testCRLF("one line\n", "one line\r\n");
    testCRLF("multiple\nlines\n", "multiple\r\nlines\r\n");
    testCRLF("empty lines\n\n", "empty lines\r\n\r\n");
    testCRLF("\ninitial line", "\r\ninitial line");
    testCRLF("mixed\r\nlines\n", "mixed\r\nlines\r\n");

    testCRLF("cropped\r", "cropped\r\n");
    testCRLF("old\rmac\rformat", "old\r\nmac\r\nformat");

    return 0;
}
