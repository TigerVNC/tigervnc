/* Copyright 2025 Adam Halim for Cendio AB
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

#include <pwd.h>
#include <unistd.h>

#include "parameters.h"

static const char* defaultDesktopName();

core::IntParameter
  rfbport("rfbport",
          "TCP port to listen for RFB protocol", 5900, -1, 65535);
core::StringParameter
  rfbunixpath("rfbunixpath",
              "Unix socket to listen for RFB protocol", "");
core::IntParameter
  rfbunixmode("rfbunixmode",
              "Unix socket access mode", 0600, 0000, 0777);
core::BoolParameter
  localhostOnly("localhost",
                "Only allow connections from localhost", false);
core::StringParameter
  desktopName("desktop", "Name of VNC desktop", defaultDesktopName());
core::StringParameter
  interface("interface",
            "Listen on the specified network address", "all");

static const char* defaultDesktopName()
{
  long host_max = sysconf(_SC_HOST_NAME_MAX);
  if (host_max < 0)
    return "";

  std::vector<char> hostname(host_max + 1);
  if (gethostname(hostname.data(), hostname.size()) == -1)
    return "";

  struct passwd* pwent = getpwuid(getuid());
  if (pwent == nullptr)
    return "";

  int len = snprintf(nullptr, 0, "%s@%s", pwent->pw_name, hostname.data());
  if (len < 0)
    return "";

  char* name = new char[len + 1];

  snprintf(name, len + 1, "%s@%s", pwent->pw_name, hostname.data());

  return name;
}
