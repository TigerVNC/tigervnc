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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <os/os.h>

#include <assert.h>

#ifndef WIN32
#include <pwd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <windows.h>
#include <wininet.h> /* MinGW needs it */
#include <shlobj.h>
#endif

static const char* gethomedir(bool userDir)
{
  static char dir[PATH_MAX];

#ifndef WIN32
  char *homedir;
  uid_t uid;
  struct passwd *passwd;
#else
  BOOL ret;
#endif

#ifndef WIN32
  homedir = getenv("HOME");
  if (homedir == NULL) {
    uid = getuid();
    passwd = getpwuid(uid);
    if (passwd == NULL) {
      /* Do we want emit error msg here? */
      return NULL;
    }
    homedir = passwd->pw_dir;
  }

  if (userDir)
    return homedir;

  snprintf(dir, sizeof(dir), "%s/.vnc", homedir);

  return dir;
#else
  if (userDir)
    ret = SHGetSpecialFolderPath(NULL, dir, CSIDL_PROFILE, FALSE);
  else
    ret = SHGetSpecialFolderPath(NULL, dir, CSIDL_APPDATA, FALSE);

  if (ret == FALSE)
    return NULL;

  if (userDir)
    return dir;

  if (strlen(dir) + strlen("\\vnc") >= sizeof(dir))
    return NULL;

  strcat(dir, "\\vnc");

  return dir;
#endif
}

const char* os::getvnchomedir()
{
  return gethomedir(false);
}

const char* os::getuserhomedir()
{
  return gethomedir(true);
}

