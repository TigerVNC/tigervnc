/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#ifndef __RFB_HOSTNAME_H__
#define __RFB_HOSTNAME_H__

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <rdr/Exception.h>
#include <rfb/util.h>

namespace rfb {

  static bool isAllSpace(const char *string) {
    if (string == NULL)
      return false;
    while(*string != '\0') {
      if (! isspace(*string))
        return false;
      string++;
    }
    return true;
  }

  static void getHostAndPort(const char* hi, char** host, int* port, int basePort=5900) {
    const char* hostStart;
    const char* hostEnd;
    const char* portStart;

    if (hi == NULL)
      throw rdr::Exception("NULL host specified");

    // Trim leading whitespace
    while(isspace(*hi))
      hi++;

    assert(host);
    assert(port);

    if (hi[0] == '[') {
      hostStart = &hi[1];
      hostEnd = strchr(hostStart, ']');
      if (hostEnd == NULL)
        throw rdr::Exception("unmatched [ in host");

      portStart = hostEnd + 1;
      if (isAllSpace(portStart))
        portStart = NULL;
    } else {
      hostStart = &hi[0];
      hostEnd = strrchr(hostStart, ':');

      if (hostEnd == NULL) {
        hostEnd = hostStart + strlen(hostStart);
        portStart = NULL;
      } else {
        if ((hostEnd > hostStart) && (hostEnd[-1] == ':'))
          hostEnd--;
        portStart = strchr(hostStart, ':');
        if (portStart != hostEnd) {
          // We found more : in the host. This is probably an IPv6 address
          hostEnd = hostStart + strlen(hostStart);
          portStart = NULL;
        }
      }
    }

    // Back up past trailing space
    while(isspace(*(hostEnd - 1)) && hostEnd > hostStart)
      hostEnd--;

    if (hostStart == hostEnd)
      *host = strDup("localhost");
    else {
      size_t len;
      len = hostEnd - hostStart + 1;
      *host = new char[len];
      strncpy(*host, hostStart, len-1);
      (*host)[len-1] = '\0';
    }

    if (portStart == NULL)
      *port = basePort;
    else {
      char* end;

      if (portStart[0] != ':')
        throw rdr::Exception("invalid port specified");

      if (portStart[1] != ':')
        *port = strtol(portStart + 1, &end, 10);
      else
        *port = strtol(portStart + 2, &end, 10);
      if (*end != '\0' && ! isAllSpace(end))
        throw rdr::Exception("invalid port specified");

      if ((portStart[1] != ':') && (*port < 100))
        *port += basePort;
    }
  }

};

#endif // __RFB_HOSTNAME_H__
