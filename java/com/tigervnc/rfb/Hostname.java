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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

public class Hostname {

  public static String getHost(String vncServerName) {
    int colonPos = vncServerName.indexOf(':');
    if (colonPos == 0)
      return "localhost";
    if (colonPos == -1)
      colonPos = vncServerName.length();
    return vncServerName.substring(0, colonPos);
  }

  public static int getPort(String vncServerName) {
    int colonPos = vncServerName.indexOf(':');
    if (colonPos == -1 || colonPos == vncServerName.length()-1)
      return 5900;
    if (vncServerName.charAt(colonPos+1) == ':') {
      return Integer.parseInt(vncServerName.substring(colonPos+2));
    }
    int port = Integer.parseInt(vncServerName.substring(colonPos+1));
    if (port < 100)
      port += 5900;
    return port;
  }
}
