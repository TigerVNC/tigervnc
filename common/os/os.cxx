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
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int gethomedir(char **dirp)
{
	char *homedir, *dir;
	size_t len;
	uid_t uid;
	struct passwd *passwd;

	assert(dirp != NULL && *dirp == NULL);

#ifdef WIN32
	/* Not supported, yet */
	return -1;
#endif

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

	len = strlen(homedir) + 1;
	dir = new char[len];
	memcpy(dir, homedir, len);

	*dirp = dir;
	return 0;
}

