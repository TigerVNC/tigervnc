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
//
// All-new VNC viewer for X.
//

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <network/TcpSocket.h>
#include "TXWindow.h"
#include "TXMsgBox.h"
#include "CConn.h"

#include "gettext.h"
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

rfb::LogWriter vlog("main");

using namespace network;
using namespace rfb;
using namespace std;

IntParameter pointerEventInterval("PointerEventInterval",
                                  "Time in milliseconds to rate-limit"
                                  " successive pointer events", 0);
IntParameter wmDecorationWidth("WMDecorationWidth", "Width of window manager "
                               "decoration around a window", 6);
IntParameter wmDecorationHeight("WMDecorationHeight", "Height of window "
                                "manager decoration around a window", 24);
StringParameter passwordFile("PasswordFile",
                             "Password file for VNC authentication", "");
AliasParameter rfbauth("passwd", "Alias for PasswordFile", &passwordFile);

BoolParameter useLocalCursor("UseLocalCursor",
                             "Render the mouse cursor locally", true);
BoolParameter dotWhenNoCursor("DotWhenNoCursor",
                              "Show the dot cursor when the server sends an "
                              "invisible cursor", true);
BoolParameter autoSelect("AutoSelect",
			"Auto select pixel format and encoding. "
			 "Default if PreferredEncoding and FullColor are not specified.", 
			 true);
BoolParameter fullColour("FullColor",
                         "Use full color", true);
AliasParameter fullColourAlias("FullColour", "Alias for FullColor", &fullColour);
IntParameter lowColourLevel("LowColorLevel",
                            "Color level to use on slow connections. "
                            "0 = Very Low (8 colors), 1 = Low (64 colors), "
                            "2 = Medium (256 colors)", 2);
AliasParameter lowColourLevelAlias("LowColourLevel", "Alias for LowColorLevel", &lowColourLevel);
StringParameter preferredEncoding("PreferredEncoding",
                                  "Preferred encoding to use (Tight, ZRLE, Hextile or"
                                  " Raw)", "Tight");
BoolParameter fullScreen("FullScreen", "Full screen mode", false);
BoolParameter viewOnly("ViewOnly",
                       "Don't send any mouse or keyboard events to the server",
                       false);
BoolParameter shared("Shared",
                     "Don't disconnect other viewers upon connection - "
                     "share the desktop instead",
                     false);
BoolParameter acceptClipboard("AcceptClipboard",
                              "Accept clipboard changes from the server",
                              true);
BoolParameter sendClipboard("SendClipboard",
                            "Send clipboard changes to the server", true);
BoolParameter sendPrimary("SendPrimary",
                          "Send the primary selection and cut buffer to the "
                          "server as well as the clipboard selection",
                          true);

BoolParameter listenMode("listen", "Listen for connections from VNC servers",
                         false);
StringParameter geometry("geometry", "X geometry specification", "");
StringParameter displayname("display", "The X display", "");

StringParameter via("via", "Gateway to tunnel via", "");

BoolParameter customCompressLevel("CustomCompressLevel",
				 "Use custom compression level. "
				 "Default if CompressLevel is specified.", false);

IntParameter compressLevel("CompressLevel",
			   "Use specified compression level"
			   "0 = Low, 9 = High",
			   6);

BoolParameter noJpeg("NoJPEG",
		     "Disable lossy JPEG compression in Tight encoding.",
		     false);

IntParameter qualityLevel("QualityLevel",
			  "JPEG quality level. "
			  "0 = Low, 9 = High",
			  6);

char aboutText[1024];
char* programName;
extern char buildtime[];

static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  vlog.info("CleanupSignalHandler called");
  exit(1);
}

// XLoginIconifier is a class which iconifies the XDM login window when it has
// grabbed the keyboard, thus releasing the grab, allowing the viewer to use
// the keyboard.  It remaps the xlogin window on exit.
class XLoginIconifier {
public:
  Display* dpy;
  Window xlogin;
  XLoginIconifier() : dpy(0), xlogin(0) {}
  void iconify(Display* dpy_) {
    dpy = dpy_;
    if (XGrabKeyboard(dpy, DefaultRootWindow(dpy), False, GrabModeSync,
                      GrabModeSync, CurrentTime) == GrabSuccess) {
      XUngrabKeyboard(dpy, CurrentTime);
    } else {
      xlogin = TXWindow::windowWithName(dpy, DefaultRootWindow(dpy), "xlogin");
      if (xlogin) {
        XIconifyWindow(dpy, xlogin, DefaultScreen(dpy));
        XSync(dpy, False);
      }
    }
  }
  ~XLoginIconifier() {
    if (xlogin) {
      fprintf(stderr,"~XLoginIconifier remapping xlogin\n");
      XMapWindow(dpy, xlogin);
      XFlush(dpy);
      sleep(1);
    }
  }
};

static XLoginIconifier xloginIconifier;

static void usage()
{
  fprintf(stderr,
          "\nusage: %s [parameters] [host:displayNum] [parameters]\n"
          "       %s [parameters] -listen [port] [parameters]\n",
          programName,programName);
  fprintf(stderr,"\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n");
  Configuration::listParams(79, 14);
  exit(1);
}

/* Tunnelling support. */
static void
interpretViaParam (char **gatewayHost, char **remoteHost,
		   int *remotePort, char **vncServerName,
		   int localPort)
{
  const int SERVER_PORT_OFFSET = 5900;
  char *pos = strchr (*vncServerName, ':');
  if (pos == NULL)
    *remotePort = SERVER_PORT_OFFSET;
  else {
    int portOffset = SERVER_PORT_OFFSET;
    size_t len;
    *pos++ = '\0';
    len = strlen (pos);
    if (*pos == ':') {
      /* Two colons is an absolute port number, not an offset. */
      pos++;
      len--;
      portOffset = 0;
    }
    if (!len || strspn (pos, "-0123456789") != len )
      usage ();
    *remotePort = atoi (pos) + portOffset;
  }

  if (**vncServerName != '\0')
    *remoteHost = *vncServerName;

  *gatewayHost = strDup (via.getValueStr ());
  *vncServerName = new char[50];
  sprintf (*vncServerName, "localhost::%d", localPort);
}

#ifndef HAVE_SETENV
int
setenv(const char *envname, const char * envval, int overwrite)
{
  if (envname && envval) {
    char * envp = NULL;
    envp = (char*)malloc(strlen(envname) + strlen(envval) + 2);
    if (envp) {
      // The putenv API guarantees memory leaks when
      // changing environment variables repeatedly.
      sprintf(envp, "%s=%s", envname, envval);
      
      // Cannot free envp
      putenv(envp);
      return(0);
    }
  }
  return(-1);
}
#endif

static void
createTunnel (const char *gatewayHost, const char *remoteHost,
	      int remotePort, int localPort)
{
  char *cmd = getenv ("VNC_VIA_CMD");
  char *percent;
  char lport[10], rport[10];
  sprintf (lport, "%d", localPort);
  sprintf (rport, "%d", remotePort);
  setenv ("G", gatewayHost, 1);
  setenv ("H", remoteHost, 1);
  setenv ("R", rport, 1);
  setenv ("L", lport, 1);
  if (!cmd)
    cmd = "/usr/bin/ssh -f -L \"$L\":\"$H\":\"$R\" \"$G\" sleep 20";
  /* Compatibility with TightVNC's method. */
  while ((percent = strchr (cmd, '%')) != NULL)
    *percent = '$';
  system (cmd);
}

int main(int argc, char** argv)
{
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE_NAME, LOCALEDIR);
  textdomain(PACKAGE_NAME);

  const char englishAbout[] = N_("TightVNC Viewer for X version %s - built %s\n"
				 "Copyright (C) 2002-2005 RealVNC Ltd.\n"
				 "Copyright (C) 2000-2006 TightVNC Group\n"
				 "Copyright (C) 2004-2005 Peter Astrand, Cendio AB\n"
				 "See http://www.tightvnc.com for information on TightVNC.");

  // Write about text to console, still using normal locale codeset
  snprintf(aboutText, sizeof(aboutText),
	   gettext(englishAbout), PACKAGE_VERSION, buildtime);
  fprintf(stderr,"\n%s\n", aboutText);

  // Set gettext codeset to what our GUI toolkit uses. Since we are
  // passing strings from strerror/gai_strerror to the GUI, these must
  // be in GUI codeset as well.
  bind_textdomain_codeset(PACKAGE_NAME, "iso-8859-1");
  bind_textdomain_codeset("libc", "iso-8859-1");

  // Re-create the aboutText for the GUI, now using GUI codeset
  snprintf(aboutText, sizeof(aboutText),
	   gettext(englishAbout), PACKAGE_VERSION, buildtime);

  rfb::initStdIOLoggers();
  rfb::LogWriter::setLogParams("*:stderr:30");

  signal(SIGHUP, CleanupSignalHandler);
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  programName = argv[0];
  char* vncServerName = 0;
  Display* dpy = 0;

  for (int i = 1; i < argc; i++) {
    if (Configuration::setParam(argv[i]))
      continue;

    if (argv[i][0] == '-') {
      if (i+1 < argc) {
        if (Configuration::setParam(&argv[i][1], argv[i+1])) {
          i++;
          continue;
        }
      }
      usage();
    }

    vncServerName = argv[i];
  }

  // Create .vnc in the user's home directory if it doesn't already exist
  char* homeDir = getenv("HOME");
  if (homeDir) {
    CharArray vncDir(strlen(homeDir)+6);
    sprintf(vncDir.buf, "%s/.vnc", homeDir);
    int result =  mkdir(vncDir.buf, 0755);
    if (result == -1 && errno != EEXIST)
      vlog.error("Could not create .vnc directory: %s.", strerror(errno));
  } else
    vlog.error("Could not create .vnc directory: environment variable $HOME not set.");

  if (!::autoSelect.hasBeenSet()) {
    // Default to AutoSelect=0 if -PreferredEncoding or -FullColor is used
    ::autoSelect.setParam(!::preferredEncoding.hasBeenSet() 
			&& !::fullColour.hasBeenSet()
			&& !::fullColourAlias.hasBeenSet());
  }
  if (!::customCompressLevel.hasBeenSet()) {
    // Default to CustomCompressLevel=1 if CompressLevel is used.
    ::customCompressLevel.setParam(::compressLevel.hasBeenSet());
  }

  try {
    /* Tunnelling support. */
    if (strlen (via.getValueStr ()) > 0) {
      char *gatewayHost = "";
      char *remoteHost = "localhost";
      int localPort = findFreeTcpPort ();
      int remotePort;
      if (!vncServerName)
        usage();
      interpretViaParam (&gatewayHost, &remoteHost, &remotePort,
			 &vncServerName, localPort);
      createTunnel (gatewayHost, remoteHost, remotePort, localPort);
    }

    Socket* sock = 0;

    if (listenMode) {
      int port = 5500;
      if (vncServerName && isdigit(vncServerName[0]))
        port = atoi(vncServerName);

      TcpListener listener(port);

      vlog.info("Listening on port %d\n",port);

      while (true) {
        sock = listener.accept();
        int pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) break; // child
        delete sock;
        int status;
        while (wait3(&status, WNOHANG, 0) > 0) ;
      }
    }

    CharArray displaynameStr(displayname.getData());
    if (!(dpy = XOpenDisplay(TXWindow::strEmptyToNull(displaynameStr.buf)))) {
      fprintf(stderr,"%s: unable to open display \"%s\"\n",
              programName, XDisplayName(displaynameStr.buf));
      exit(1);
    }

    TXWindow::init(dpy, "Vncviewer");
    xloginIconifier.iconify(dpy);
    CConn cc(dpy, argc, argv, sock, vncServerName, listenMode);

    // X events are processed whenever reading from the socket would block.

    while (true) {
      cc.getInStream()->check(1);
      cc.processMsg();
    }

  } catch (rdr::EndOfStream& e) {
    vlog.info(e.str());
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
    if (dpy) {
      TXMsgBox msgBox(dpy, e.str(), MB_OK, "VNC Viewer: Information");
      msgBox.show();
    }
    return 1;
  }

  return 0;
}
