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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <pwd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#else
#include <windows.h>
#include <wininet.h> /* MinGW needs it */
#include <shlobj.h>
#define stat _stat
#define mkdir(path, mode) mkdir(path)
#endif

static const char* getvncdir(bool userDir, const char *xdg_env, const char *xdg_def)
{
  static char dir[PATH_MAX], legacy[PATH_MAX];
  struct stat st;

#ifndef WIN32
  char *homedir, *xdgdir;
  uid_t uid;
  struct passwd *passwd;
#else
  BOOL ret;
#endif

#ifndef WIN32
  homedir = getenv("HOME");
  if (homedir == nullptr) {
    uid = getuid();
    passwd = getpwuid(uid);
    if (passwd == nullptr) {
      /* Do we want emit error msg here? */
      return nullptr;
    }
    homedir = passwd->pw_dir;
  }

  if (userDir)
    return homedir;

  xdgdir = getenv(xdg_env);
  if (xdgdir != nullptr && xdgdir[0] == '/')
    snprintf(dir, sizeof(dir), "%s/tigervnc", xdgdir);
  else
    snprintf(dir, sizeof(dir), "%s/%s/tigervnc", homedir, xdg_def);

  snprintf(legacy, sizeof(legacy), "%s/.vnc", homedir);
#else
  (void) xdg_def;
  (void) xdg_env;

  if (userDir)
    ret = SHGetSpecialFolderPath(nullptr, dir, CSIDL_PROFILE, FALSE);
  else
    ret = SHGetSpecialFolderPath(nullptr, dir, CSIDL_APPDATA, FALSE);

  if (ret == FALSE)
    return nullptr;

  if (userDir)
    return dir;

  ret = SHGetSpecialFolderPath(nullptr, legacy, CSIDL_APPDATA, FALSE);

  if (ret == FALSE)
    return nullptr;

  if (strlen(dir) + strlen("\\TigerVNC") >= sizeof(dir))
    return nullptr;
  if (strlen(legacy) + strlen("\\vnc") >= sizeof(legacy))
    return nullptr;

  strcat(dir, "\\TigerVNC");
  strcat(legacy, "\\vnc");
#endif
  return (stat(dir, &st) != 0 && stat(legacy, &st) == 0) ? legacy : dir;
}

const char* os::getuserhomedir()
{
  return getvncdir(true, nullptr, nullptr);
}

const char* os::getvncconfigdir()
{
  return getvncdir(false, "XDG_CONFIG_HOME", ".config");
}

const char* os::getvncdatadir()
{
  return getvncdir(false, "XDG_DATA_HOME", ".local/share");
}

const char* os::getvncstatedir()
{
  return getvncdir(false, "XDG_STATE_HOME", ".local/state");
}

int os::mkdir_p(const char *path_, mode_t mode)
{
  char *path = strdup(path_);
  char *p;

#ifdef WIN32
  (void)mode;
#endif

  for (p = path + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(path, mode) == -1) {
        if (errno != EEXIST) {
          free(path);
          return -1;
        }
      }
      *p = '/';
    }
  }

  if (mkdir(path, mode) == -1) {
    free(path);
    return -1;
  }

  free(path);

  return 0;
}
