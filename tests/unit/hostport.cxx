/* Copyright 2016 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <network/TcpSocket.h>

static void doTest(const char* hostAndPort,
                   const char* expectedHost, int expectedPort)
{
    std::string host;
    int port;

    printf("\"%s\": ", hostAndPort);

    network::getHostAndPort(hostAndPort, &host, &port);

    if (host != expectedHost)
        printf("FAILED (\"%s\" != \"%s\")", host.c_str(), expectedHost);
    else if (port != expectedPort)
        printf("FAILED (%d != %d)", port, expectedPort);
    else
        printf("OK");
    printf("\n");
    fflush(stdout);
}

int main(int /*argc*/, char** /*argv*/)
{
    doTest(":5", "localhost", 5905);

    doTest("1.2.3.4", "1.2.3.4", 5900);

    doTest("1.2.3.4:5", "1.2.3.4", 5905);
    doTest("1.2.3.4:99", "1.2.3.4", 5999);
    doTest("1.2.3.4:100", "1.2.3.4", 100);
    doTest("1.2.3.4:5901", "1.2.3.4", 5901);

    doTest("1.2.3.4::5", "1.2.3.4", 5);
    doTest("1.2.3.4::99", "1.2.3.4", 99);
    doTest("1.2.3.4::5901", "1.2.3.4", 5901);

    doTest("[1.2.3.4]", "1.2.3.4", 5900);
    doTest("[1.2.3.4]:5", "1.2.3.4", 5905);
    doTest("[1.2.3.4]:100", "1.2.3.4", 100);
    doTest("[1.2.3.4]::5", "1.2.3.4", 5);
    doTest("[1.2.3.4]::100", "1.2.3.4", 100);

    // Ambigiuous. For now we'll keep the old behaviour...
    doTest("::1", "localhost", 1);

    doTest("2001:1234::20:1", "2001:1234::20:1", 5900);

    doTest("[::1]", "::1", 5900);
    doTest("[2001:1234::20:1]", "2001:1234::20:1", 5900);

    doTest("[2001:1234::20:1]:5", "2001:1234::20:1", 5905);
    doTest("[2001:1234::20:1]:99", "2001:1234::20:1", 5999);
    doTest("[2001:1234::20:1]:100", "2001:1234::20:1", 100);
    doTest("[2001:1234::20:1]:5901", "2001:1234::20:1", 5901);

    doTest("    1.2.3.4    ", "1.2.3.4", 5900);
    doTest("    1.2.3.4:5901    ", "1.2.3.4", 5901);
    doTest("    1.2.3.4:   5901    ", "1.2.3.4", 5901);
    doTest("    1.2.3.4    :5901    ", "1.2.3.4", 5901);
    doTest("    [1.2.3.4]:5902    ", "1.2.3.4", 5902);
    doTest("    :5903    ", "localhost", 5903);
    doTest("    ::4    ", "localhost", 4);
    doTest("    [::1]    ", "::1", 5900);
    doTest("    2001:1234::20:1    ", "2001:1234::20:1", 5900);
    doTest("    [2001:1234::20:1]    ", "2001:1234::20:1", 5900);
    doTest("    [2001:1234::20:1]:5905    ", "2001:1234::20:1", 5905);

    return 0;
}
