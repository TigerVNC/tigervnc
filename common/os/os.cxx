/* Copyright (C) 2010 TightVNC Team.  All Rights Reserved.
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <windows.h>
#include <wininet.h> /* MinGW needs it */
#include <shlobj.h>
#endif

static int gethomedir(char **dirp, bool userDir)
{
#ifndef WIN32
	char *homedir, *dir;
	size_t len;
	uid_t uid;
	struct passwd *passwd;
#else
	TCHAR *dir;
	BOOL ret;
#endif

	assert(dirp != NULL && *dirp == NULL);

#ifndef WIN32
	homedir = getenv("HOME");
	if (homedir == NULL) {
		uid = getuid();
		passwd = getpwuid(uid);
		if (passwd == NULL) {
			/* Do we want emit error msg here? */
			return -1;
		}
		homedir = passwd->pw_dir;
	}

	len = strlen(homedir);
	dir = new char[len+7];
	if (dir == NULL)
		return -1;

	memcpy(dir, homedir, len);
	if (userDir)
		dir[len]='\0';
	else
		memcpy(dir + len, "/.vnc/\0", 7);
#else
	dir = new TCHAR[MAX_PATH];
	if (dir == NULL)
		return -1;

	if (userDir)
		ret = SHGetSpecialFolderPath(NULL, dir, CSIDL_PROFILE, FALSE);
	else
		ret = SHGetSpecialFolderPath(NULL, dir, CSIDL_APPDATA, FALSE);

	if (ret == FALSE) {
		delete [] dir;
		return -1;
	}
	if (userDir)
		dir[strlen(dir)+1] = '\0';
	else
		memcpy(dir+strlen(dir), (TCHAR *)"\\vnc\\\0", 6);
#endif
	*dirp = dir;
	return 0;
}

int getvnchomedir(char **dirp)
{
	return gethomedir(dirp, false);
}

int getuserhomedir(char **dirp)
{
	return gethomedir(dirp, true);
}

