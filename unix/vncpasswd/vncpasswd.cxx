/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 Antoine Martin.  All Rights Reserved.
 * Copyright (C) 2010 D. R. Commander.  All Rights Reserved.
 * Copyright 2018-2023 Pierre Ossman for Cendio AB
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <os/os.h>

#include <rfb/obfuscate.h>

#include <termios.h>

#ifdef HAVE_PWQUALITY
#include <pwquality.h>
#endif

using namespace rfb;

char* prog;

static void usage()
{
  fprintf(stderr,"usage: %s [file]\n", prog);
  fprintf(stderr,"       %s -f\n", prog);
  exit(1);
}


static void enableEcho(bool enable) {
  termios attrs;
  tcgetattr(fileno(stdin), &attrs);
  if (enable)
    attrs.c_lflag |= ECHO;
  else
    attrs.c_lflag &= ~ECHO;
  attrs.c_lflag |= ECHONL;
  tcsetattr(fileno(stdin), TCSAFLUSH, &attrs);
}

static const char* getpassword(const char* prompt) {
  static char buf[256];
  if (prompt) fputs(prompt, stdout);
  enableEcho(false);
  char* result = fgets(buf, 256, stdin);
  enableEcho(true);
  if (result) {
    if (result[strlen(result)-1] == '\n')
      result[strlen(result)-1] = 0;
    return buf;
  }
  return nullptr;
}

// Reads passwords from stdin and prints encrypted passwords to stdout.
static int encrypt_pipe() {
  int i;

  // We support a maximum of two passwords right now
  for (i = 0;i < 2;i++) {
    const char *result = getpassword(nullptr);
    if (!result)
      break;

    std::vector<uint8_t> obfuscated = obfuscate(result);
    if (fwrite(obfuscated.data(), obfuscated.size(), 1, stdout) != 1) {
      fprintf(stderr,"Writing to stdout failed\n");
      return 1;
    }
  }

  // Did we fail to produce even one password?
  if (i == 0)
    return 1;

  return 0;
}

#ifdef HAVE_PWQUALITY
static int check_passwd_pwquality(const char *password)
{
	int r;
	void *auxerror;
	pwquality_settings_t *pwq;
	pwq = pwquality_default_settings();
	if (!pwq)
		return -EINVAL;
	r = pwquality_read_config(pwq, NULL, &auxerror);
	if (r) {
		printf("Cannot check password quality: %s \n",
			pwquality_strerror(NULL, 0, r, auxerror));
		pwquality_free_settings(pwq);
		return -EINVAL;
	}

	r = pwquality_check(pwq, password, NULL, NULL, &auxerror);
	if (r < 0) {
		printf("Password quality check failed:\n %s \n",
			pwquality_strerror(NULL, 0, r, auxerror));
		r = -EPERM;
	}
	pwquality_free_settings(pwq);

	//return the score of password quality
	return r;
}
#endif

static std::vector<uint8_t> readpassword() {
  while (true) {
    const char *passwd = getpassword("Password:");
    if (passwd == nullptr) {
      perror("getpassword error");
      exit(1);
    }

    std::string first = passwd;

    if (first.empty()) {
      fprintf(stderr,"Password not changed\n");
      exit(1);
    }

    if (first.size() > 8) {
      fprintf(stderr,"Password should not be greater than 8 characters\nBecause only 8 valid characters are used - try again\n");
      continue;
    }

#ifdef HAVE_PWQUALITY
    //the function return score of password quality
    int r = check_passwd_pwquality(passwd);
    if (r < 0){
      printf("Password quality check failed, please set it correctly.\n");
      continue;
    }
#else
    if (first.size() < 6) {
      fprintf(stderr,"Password must be at least 6 characters - try again\n");
      continue;
    }
#endif

    passwd = getpassword("Verify:");
    if (passwd == nullptr) {
      perror("getpass error");
      exit(1);
    }
    std::string second = passwd;
    if (first != second) {
      fprintf(stderr,"Passwords don't match - try again\n");
      continue;
    }

    return obfuscate(first.c_str());
  }
}

int main(int argc, char** argv)
{
  prog = argv[0];

  char fname[PATH_MAX];

  fname[0] = '\0';

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-q") == 0) { // allowed for backwards compatibility
    } else if (strncmp(argv[i], "-f", 2) == 0) {
      return encrypt_pipe();
    } else if (argv[i][0] == '-') {
      usage();
    } else if (fname[0] == '\0') {
      if (strlen(argv[i]) >= sizeof(fname)) {
        fprintf(stderr, "Too long filename specified\n");
        return -1;
      }
      strcpy(fname, argv[i]);
    } else {
      usage();
    }
  }

  if (fname[0] == '\0') {
    const char *configDir = os::getvncconfigdir();
    if (configDir == nullptr) {
      fprintf(stderr, "Can't obtain VNC config directory\n");
      exit(1);
    }
    if (os::mkdir_p(configDir, 0777) == -1) {
      if (errno != EEXIST) {
        fprintf(stderr, "Could not create VNC config directory: %s\n", strerror(errno));
        exit(1);
      }
    }
    snprintf(fname, sizeof(fname), "%s/passwd", configDir);
  }

  while (true) {
    std::vector<uint8_t> obfuscated = readpassword();
    std::vector<uint8_t> obfuscatedReadOnly;

    fprintf(stderr, "Would you like to enter a view-only password (y/n)? ");
    char yesno[3];
    if (fgets(yesno, 3, stdin) != nullptr && (yesno[0] == 'y' || yesno[0] == 'Y')) {
      obfuscatedReadOnly = readpassword();
    } else {
      fprintf(stderr, "A view-only password is not used\n");
    }

    FILE* fp = fopen(fname,"w");
    if (!fp) {
      fprintf(stderr,"Couldn't open %s for writing\n",fname);
      exit(1);
    }
    chmod(fname, S_IRUSR|S_IWUSR);

    if (fwrite(obfuscated.data(), obfuscated.size(), 1, fp) != 1) {
      fprintf(stderr,"Writing to %s failed\n",fname);
      exit(1);
    }

    if (!obfuscatedReadOnly.empty()) {
      if (fwrite(obfuscatedReadOnly.data(), obfuscatedReadOnly.size(), 1, fp) != 1) {
        fprintf(stderr,"Writing to %s failed\n",fname);
        exit(1);
      }
    }

    fclose(fp);

    return 0;
  }
}
