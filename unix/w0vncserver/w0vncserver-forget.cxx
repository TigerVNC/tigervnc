
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>

#include <core/xdgdirs.h>
#include <core/Configuration.h>

// Sync with RemoteDesktop.cxx
static const char* RESTORE_TOKEN_FILENAME =  "restoretoken";
static const char* CLIPBOARD_PREFERENCE_FILENAME =  "clipboard";

const char* filepaths[] = { RESTORE_TOKEN_FILENAME,
                            CLIPBOARD_PREFERENCE_FILENAME };

char* programName = nullptr;

static void printVersion(FILE *fp)
{
  fprintf(fp, "TigerVNC server version %s\n", PACKAGE_VERSION);
}

static void usage()
{
  printVersion(stderr);
  fprintf(stderr,"Usage: %s\n", programName);
  exit(1);
}

int main(int argc, char** argv)
{
  const char* stateDir;
  int ret;

  programName = argv[0];

  for (int i = 1; i < argc; i++) {
   if ((strcmp(argv[i], "-h") == 0) ||
       (strcmp(argv[i], "--help") == 0)) {
      usage();
    } else if ((strcmp(argv[i], "-v") == 0) ||
               (strcmp(argv[i], "--version") == 0)) {
      printVersion(stderr);
      exit(0);
    } else {
      fprintf(stderr, "%s: Extra argument '%s'\n", programName, argv[i]);
      fprintf(stderr, "See '%s --help' for more information.\n",
              programName);
      exit(1);
    }
  }

  stateDir = core::getvncstatedir();
  if (!stateDir) {
    fprintf(stderr, "Could not get state directory");
    return 1;
  }

  ret = 0;
  for (uint i = 0; i < sizeof(filepaths) / sizeof(filepaths[0]); i++) {
    char filepath[PATH_MAX];

    snprintf(filepath, sizeof(filepath), "%s/%s", stateDir, filepaths[i]);

    if (remove(filepath) != 0) {
      if (errno != ENOENT) {
        fprintf(stderr, "Could not remove preference from \"%s\": %s",
                filepath, strerror(errno));
       ret = 1;
      }
    }
  }

  return ret;
}
