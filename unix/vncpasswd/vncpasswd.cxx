/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 Antoine Martin.  All Rights Reserved.
 * Copyright (C) 2010 D. R. Commander.  All Rights Reserved.
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <os/os.h>
#include <rfb/Password.h>
#include <rfb/util.h>

#include <termios.h>


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

static char* getpassword(const char* prompt) {
  PlainPasswd buf(256);
  if (prompt) fputs(prompt, stdout);
  enableEcho(false);
  char* result = fgets(buf.buf, 256, stdin);
  enableEcho(true);
  if (result) {
    if (result[strlen(result)-1] == '\n')
      result[strlen(result)-1] = 0;
    return buf.takeBuf();
  }
  return 0;
}

// Reads password from stdin and prints encrypted password to stdout.
static int encrypt_pipe() {
  char *result = getpassword(NULL);
  if (result) {
    ObfuscatedPasswd obfuscated(result);
    if (fwrite(obfuscated.buf, obfuscated.length, 1, stdout) != 1) {
      fprintf(stderr,"Writing to stdout failed\n");
      return 1;
    }
    return 0;
  }
  else return 1;
}

int main(int argc, char** argv)
{
  prog = argv[0];

  char* fname = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-q") == 0) { // allowed for backwards compatibility
    } else if (strncmp(argv[i], "-f", 2) == 0) {
      return encrypt_pipe();
    } else if (argv[i][0] == '-') {
      usage();
    } else if (!fname) {
      fname = argv[i];
    } else {
      usage();
    }
  }

  if (!fname) {
    char *homeDir = NULL;
    if (gethomedir(&homeDir) == -1) {
      fprintf(stderr, "Can't obtain home directory\n");
      exit(1);
    }
    fname = new char[strlen(homeDir) + 20];
    sprintf(fname, "%s/.vnc", homeDir);
    mkdir(fname, 0777);
    sprintf(fname, "%s/.vnc/passwd", homeDir);
    delete [] homeDir;
  }

  while (true) {
    PlainPasswd passwd(getpassword("Password:"));
    if (!passwd.buf) {
      perror("getpassword error");
      exit(1);
    }   
    if (strlen(passwd.buf) < 6) {
      if (strlen(passwd.buf) == 0) {
        fprintf(stderr,"Password not changed\n");
        exit(1);
      }
      fprintf(stderr,"Password must be at least 6 characters - try again\n");
      continue;
    }

    PlainPasswd passwd2(getpassword("Verify:"));
    if (!passwd2.buf) {
      perror("getpass error");
      exit(1);
    }   
    if (strcmp(passwd.buf, passwd2.buf) != 0) {
      fprintf(stderr,"Passwords don't match - try again\n");
      continue;
    }

    FILE* fp = fopen(fname,"w");
    if (!fp) {
      fprintf(stderr,"Couldn't open %s for writing\n",fname);
      exit(1);
    }
    chmod(fname, S_IRUSR|S_IWUSR);

    ObfuscatedPasswd obfuscated(passwd);

    if (fwrite(obfuscated.buf, obfuscated.length, 1, fp) != 1) {
      fprintf(stderr,"Writing to %s failed\n",fname);
      exit(1);
    }

    fclose(fp);

    return 0;
  }
}
