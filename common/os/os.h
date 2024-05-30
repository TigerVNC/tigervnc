/* Copyright (C) 2010 TightVNC Team.  All Rights Reserved.
 * Copyright 2021-2023 Pierre Ossman for Cendio AB
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

#ifndef OS_OS_H
#define OS_OS_H

#include <sys/stat.h>

namespace os {

  /*
   * Get user home directory.
   * If HOME environment variable is set then it is used.
   * Otherwise home directory is obtained via getpwuid function.
   *
   * Returns NULL on failure.
   */
  const char* getuserhomedir();

  /*
   * Get VNC config directory. On Unix-like systems, this is either:
   * - $XDG_CONFIG_HOME/tigervnc
   * - $HOME/.config/tigervnc
   * On Windows, this is simply %APPDATA%/TigerVNC/.
   *
   * Returns NULL on failure.
   */
  const char* getvncconfigdir();

  /*
   * Get VNC data directory used for X.509 known hosts.
   * On Unix-like systems, this is either:
   * - $XDG_DATA_HOME/tigervnc
   * - $HOME/.local/share/tigervnc
   * On Windows, this is simply %APPDATA%/TigerVNC/.
   *
   * Returns NULL on failure.
   */
  const char* getvncdatadir();

  /*
   * Get VNC state (logs) directory. On Unix-like systems, this is either:
   * - $XDG_STATE_HOME/tigervnc
   * - $HOME/.local/state/tigervnc
   * On Windows, this is simply %APPDATA%/TigerVNC/.
   *
   * Returns NULL on failure.
   */
  const char* getvncstatedir();

  /*
   * Create directory recursively. Useful to create the nested directory
   * structures needed for the above directories.
   */
  int mkdir_p(const char *path, mode_t mode);
}

#endif /* OS_OS_H */
