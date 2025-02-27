/* Copyright 2016-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <network/TcpSocket.h>

struct result {
  std::string host;
  int port;
};

static bool operator==(const result& a, const result& b)
{
  return a.host == b.host && a.port == b.port;
}

static std::ostream& operator<<(std::ostream& os, const result& r)
{
  return os << r.host << ":" << r.port;
}

static result getHostAndPort(const char* hostAndPort)
{
  std::string host;
  int port;
  network::getHostAndPort(hostAndPort, &host, &port);
  return {host, port};
}

TEST(HostPost, localDisplay)
{
  EXPECT_EQ(getHostAndPort(":5"), result({"localhost", 5905}));
}

TEST(HostPost, noDisplay)
{
  EXPECT_EQ(getHostAndPort("1.2.3.4"), result({"1.2.3.4", 5900}));
}

TEST(HostPost, display)
{
  EXPECT_EQ(getHostAndPort("1.2.3.4:5"), result({"1.2.3.4", 5905}));
  EXPECT_EQ(getHostAndPort("1.2.3.4:99"), result({"1.2.3.4", 5999}));
  EXPECT_EQ(getHostAndPort("1.2.3.4:100"), result({"1.2.3.4", 100}));
  EXPECT_EQ(getHostAndPort("1.2.3.4:5901"), result({"1.2.3.4", 5901}));
}

TEST(HostPost, port)
{
  EXPECT_EQ(getHostAndPort("1.2.3.4::5"), result({"1.2.3.4", 5}));
  EXPECT_EQ(getHostAndPort("1.2.3.4::99"), result({"1.2.3.4", 99}));
  EXPECT_EQ(getHostAndPort("1.2.3.4::5901"), result({"1.2.3.4", 5901}));
}

TEST(HostPost, bracketedIpv4)
{
  EXPECT_EQ(getHostAndPort("[1.2.3.4]"), result({"1.2.3.4", 5900}));
  EXPECT_EQ(getHostAndPort("[1.2.3.4]:5"), result({"1.2.3.4", 5905}));
  EXPECT_EQ(getHostAndPort("[1.2.3.4]:100"), result({"1.2.3.4", 100}));
  EXPECT_EQ(getHostAndPort("[1.2.3.4]::5"), result({"1.2.3.4", 5}));
  EXPECT_EQ(getHostAndPort("[1.2.3.4]::100"), result({"1.2.3.4", 100}));
}

TEST(HostPost, portOne)
{
  // Ambigiuous. For now we'll keep the old behaviour...
  EXPECT_EQ(getHostAndPort("::1"), result({"localhost", 1}));
}

TEST(HostPost, bareIpv6)
{
  EXPECT_EQ(getHostAndPort("2001:1234::20:1"), result({"2001:1234::20:1", 5900}));
}

TEST(HostPost, bracketedIpv6)
{
  EXPECT_EQ(getHostAndPort("[::1]"), result({"::1", 5900}));
  EXPECT_EQ(getHostAndPort("[2001:1234::20:1]"), result({"2001:1234::20:1", 5900}));
}

TEST(HostPost, ipv6WithDisplay)
{
  EXPECT_EQ(getHostAndPort("[2001:1234::20:1]:5"), result({"2001:1234::20:1", 5905}));
  EXPECT_EQ(getHostAndPort("[2001:1234::20:1]:99"), result({"2001:1234::20:1", 5999}));
  EXPECT_EQ(getHostAndPort("[2001:1234::20:1]:100"), result({"2001:1234::20:1", 100}));
  EXPECT_EQ(getHostAndPort("[2001:1234::20:1]:5901"), result({"2001:1234::20:1", 5901}));
}

TEST(HostPort, padding)
{
  EXPECT_EQ(getHostAndPort("    1.2.3.4    "), result({"1.2.3.4", 5900}));
  EXPECT_EQ(getHostAndPort("    1.2.3.4:5901    "), result({"1.2.3.4", 5901}));
  EXPECT_EQ(getHostAndPort("    1.2.3.4    :5901    "), result({"1.2.3.4", 5901}));
  EXPECT_EQ(getHostAndPort("    [1.2.3.4]:5902    "), result({"1.2.3.4", 5902}));
  EXPECT_EQ(getHostAndPort("    :5903    "), result({"localhost", 5903}));
  EXPECT_EQ(getHostAndPort("    ::4    "), result({"localhost", 4}));
  EXPECT_EQ(getHostAndPort("    [::1]    "), result({"::1", 5900}));
  EXPECT_EQ(getHostAndPort("    2001:1234::20:1    "), result({"2001:1234::20:1", 5900}));
  EXPECT_EQ(getHostAndPort("    [2001:1234::20:1]    "), result({"2001:1234::20:1", 5900}));
  EXPECT_EQ(getHostAndPort("    [2001:1234::20:1]:5905    "), result({"2001:1234::20:1", 5905}));
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
