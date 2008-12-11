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

#include <vncviewer/CConnOptions.h>
#include <rfb/Configuration.h>
#include <rfb/encodings.h>
#include <rfb/LogWriter.h>
#include <rfb/ScaleFilters.h>
#include <rfb_win32/MsgBox.h>
#include <rfb_win32/Registry.h>
#include <rdr/HexInStream.h>
#include <rdr/HexOutStream.h>
#include <stdlib.h>

using namespace rfb;
using namespace rfb::win32;

static StringParameter passwordFile("PasswordFile",
				    "Password file for VNC authentication", "");

// - Settings stored in the registry & in .vnc files, by Save Defaults and
//   Save Configuration respectively.

static BoolParameter useLocalCursor("UseLocalCursor", "Render the mouse cursor locally", true);
static BoolParameter useDesktopResize("UseDesktopResize", "Support dynamic desktop resizing", true);

static BoolParameter fullColour("FullColor",
				"Use full color", true);
static AliasParameter fullColourAlias("FullColour", "Alias for FullColor", &fullColour);

static IntParameter lowColourLevel("LowColorLevel",
                         "Color level to use on slow connections. "
                         "0 = Very Low (8 colors), 1 = Low (64 colors), 2 = Medium (256 colors)",
                         2);
static AliasParameter lowColourLevelAlias("LowColourLevel", "Alias for LowColorLevel", &lowColourLevel);

static BoolParameter fullScreen("FullScreen",
                         "Use the whole display to show the remote desktop."
                         "(Press F8 to access the viewer menu)",
                         false);
static StringParameter preferredEncoding("PreferredEncoding",
					 "Preferred encoding to use (Tight, ZRLE, Hextile or"
					 " Raw)", "Tight");
static BoolParameter autoSelect("AutoSelect", 
				"Auto select pixel format and encoding. "
				"Default if PreferredEncoding and FullColor are not specified.", 
				true);
static BoolParameter sharedConnection("Shared",
                         "Allow existing connections to the server to continue."
                         "(Default is to disconnect all other clients)",
                         false);

static BoolParameter sendPtrEvents("SendPointerEvents",
                         "Send pointer (mouse) events to the server.", true);
static BoolParameter sendKeyEvents("SendKeyEvents",
                         "Send key presses (and releases) to the server.", true);

static BoolParameter clientCutText("ClientCutText",
                         "Send clipboard changes to the server.", true);
static BoolParameter serverCutText("ServerCutText",
                         "Accept clipboard changes from the server.", true);

static BoolParameter disableWinKeys("DisableWinKeys",
                         "Pass special Windows keys directly to the server.", true);

static BoolParameter protocol3_3("Protocol3.3",
                         "Only use protocol version 3.3", false);

static IntParameter ptrEventInterval("PointerEventInterval",
                         "The interval to delay between sending one pointer event "
                         "and the next.", 0);
static BoolParameter emulate3("Emulate3",
                         "Emulate middle mouse button when left and right buttons "
                         "are used simulatenously.", false);

static BoolParameter acceptBell("AcceptBell",
                         "Produce a system beep when requested to by the server.",
                         true);

static BoolParameter showToolbar("ShowToolbar", "Show toolbar by default.", true);

static StringParameter monitor("Monitor", "The monitor to open the VNC Viewer window on, if available.", "");
static StringParameter menuKey("MenuKey", "The key which brings up the popup menu", "F8");
static BoolParameter autoReconnect("AutoReconnect", "Offer to reconnect to the remote server if the connection"
                                   "is dropped because an error occurs.", true);

static BoolParameter customCompressLevel("CustomCompressLevel",
					 "Use custom compression level. "
					 "Default if CompressLevel is specified.", false);

static IntParameter compressLevel("CompressLevel",
				  "Use specified compression level"
				  "0 = Low, 9 = High",
				  6);

static BoolParameter noJpeg("NoJPEG",
			    "Disable lossy JPEG compression in Tight encoding.",
			    false);

static IntParameter qualityLevel("QualityLevel",
				 "JPEG quality level. "
				 "0 = Low, 9 = High",
				 6);

static BoolParameter autoScaling("AutoScaling",
                          "Auto rescale local copy of the remote desktop to the client window.",
                          false);
static IntParameter scale("Scale", 
                          "Scale local copy of the remote desktop, in percent",
                          100);

CConnOptions::CConnOptions()
: useLocalCursor (::useLocalCursor), 
useDesktopResize(::useDesktopResize),
fullScreen(::fullScreen), 
fullColour(::fullColour), 
lowColourLevel(::lowColourLevel), 
preferredEncoding(encodingZRLE), 
autoSelect(::autoSelect), 
shared(::sharedConnection), 
sendPtrEvents(::sendPtrEvents), 
sendKeyEvents(::sendKeyEvents),
showToolbar(::showToolbar), 
clientCutText(::clientCutText), 
serverCutText(::serverCutText),
disableWinKeys(::disableWinKeys), 
emulate3(::emulate3), 
pointerEventInterval(ptrEventInterval),
protocol3_3(::protocol3_3), 
acceptBell(::acceptBell),
autoScaling(::autoScaling), 
scale(::scale), 
monitor(::monitor.getData()),
autoReconnect(::autoReconnect), 
customCompressLevel(::customCompressLevel), 
compressLevel(::compressLevel), 
noJpeg(::noJpeg), 
qualityLevel(::qualityLevel),
passwordFile(::passwordFile.getData())
{
  if (autoSelect) {
    preferredEncoding = encodingZRLE;
  } else {
    CharArray encodingName(::preferredEncoding.getData());
    preferredEncoding = encodingNum(encodingName.buf);
  }
  setMenuKey(CharArray(::menuKey.getData()).buf);

  if (!::autoSelect.hasBeenSet()) {
      // Default to AutoSelect=0 if -PreferredEncoding or -FullColor is used
    autoSelect = (!::preferredEncoding.hasBeenSet() 
		  && !::fullColour.hasBeenSet()
		  && !::fullColourAlias.hasBeenSet());
  }
  if (!::customCompressLevel.hasBeenSet()) {
    // Default to CustomCompressLevel=1 if CompressLevel is used.
    customCompressLevel = ::compressLevel.hasBeenSet();
  }
}


void CConnOptions::readFromFile(const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f)
    throw rdr::Exception("Failed to read configuration file");

  try { 
    char line[4096];
    CharArray section;

    CharArray hostTmp;
    int portTmp = 0;

    while (!feof(f)) {
      // Read the next line
      if (!fgets(line, sizeof(line), f)) {
        if (feof(f))
          break;
        throw rdr::SystemException("fgets", ferror(f));
      }
      int len=strlen(line);
      if (line[len-1] == '\n') {
        line[len-1] = 0;
        len--;
      }

      // Process the line
      if (line[0] == ';') {
        // Comment
      } else if (line[0] == '[') {
        // Entering a new section
        if (!strSplit(&line[1], ']', &section.buf, 0))
          throw rdr::Exception("bad Section");
      } else {
        // Reading an option
        CharArray name;
        CharArray value;
        if (!strSplit(line, '=', &name.buf, &value.buf))
          throw rdr::Exception("bad Name/Value pair");

        if (stricmp(section.buf, "Connection") == 0) {
          if (stricmp(name.buf, "Host") == 0) {
            hostTmp.replaceBuf(value.takeBuf());
          } else if (stricmp(name.buf, "Port") == 0) {
            portTmp = atoi(value.buf);
          } else if (stricmp(name.buf, "UserName") == 0) {
            userName.replaceBuf(value.takeBuf());
          } else if (stricmp(name.buf, "Password") == 0) {
            ObfuscatedPasswd obfPwd;
            rdr::HexInStream::hexStrToBin(value.buf, (char**)&obfPwd.buf, &obfPwd.length);
            PlainPasswd passwd(obfPwd);
            password.replaceBuf(passwd.takeBuf());
          }
        } else if (stricmp(section.buf, "Options") == 0) {
            // V4 options
          if (stricmp(name.buf, "UseLocalCursor") == 0) {
            useLocalCursor = atoi(value.buf);
          } else if (stricmp(name.buf, "UseDesktopResize") == 0) {
            useDesktopResize = atoi(value.buf);
          } else if (stricmp(name.buf, "FullScreen") == 0) {
            fullScreen = atoi(value.buf);
          } else if (stricmp(name.buf, "FullColour") == 0) {
            fullColour = atoi(value.buf);
          } else if (stricmp(name.buf, "LowColourLevel") == 0) {
            lowColourLevel = atoi(value.buf);
          } else if (stricmp(name.buf, "PreferredEncoding") == 0) {
            preferredEncoding = encodingNum(value.buf);
          } else if ((stricmp(name.buf, "AutoDetect") == 0) ||
                     (stricmp(name.buf, "AutoSelect") == 0)) {
            autoSelect = atoi(value.buf);
          } else if (stricmp(name.buf, "Shared") == 0) {
            shared = atoi(value.buf);
          } else if (stricmp(name.buf, "SendPtrEvents") == 0) {
            sendPtrEvents = atoi(value.buf);
          } else if (stricmp(name.buf, "SendKeyEvents") == 0) {
            sendKeyEvents = atoi(value.buf);
          } else if (stricmp(name.buf, "SendCutText") == 0) {
            clientCutText = atoi(value.buf);
          } else if (stricmp(name.buf, "AcceptCutText") == 0) {
            serverCutText = atoi(value.buf);
          } else if (stricmp(name.buf, "DisableWinKeys") == 0) {
            disableWinKeys = atoi(value.buf);
          } else if (stricmp(name.buf, "AcceptBell") == 0) {
            acceptBell = atoi(value.buf);
          } else if (stricmp(name.buf, "Emulate3") == 0) {
            emulate3 = atoi(value.buf);
          } else if (stricmp(name.buf, "ShowToolbar") == 0) {
            showToolbar = atoi(value.buf);
          } else if (stricmp(name.buf, "PointerEventInterval") == 0) {
            pointerEventInterval = atoi(value.buf);
          } else if (stricmp(name.buf, "Monitor") == 0) {
            monitor.replaceBuf(value.takeBuf());
          } else if (stricmp(name.buf, "MenuKey") == 0) {
            setMenuKey(value.buf);
          } else if (stricmp(name.buf, "AutoReconnect") == 0) {
            autoReconnect = atoi(value.buf);

          } else if (stricmp(name.buf, "CustomCompressLevel") == 0) {
	    customCompressLevel = atoi(value.buf);
          } else if (stricmp(name.buf, "CompressLevel") == 0) {
	    compressLevel = atoi(value.buf);
          } else if (stricmp(name.buf, "NoJPEG") == 0) {
	    noJpeg = atoi(value.buf);
          } else if (stricmp(name.buf, "QualityLevel") == 0) {
	    qualityLevel = atoi(value.buf);
            // Legacy options
          } else if (stricmp(name.buf, "Preferred_Encoding") == 0) {
            preferredEncoding = atoi(value.buf);
          } else if (stricmp(name.buf, "8bit") == 0) {
            fullColour = !atoi(value.buf);
          } else if (stricmp(name.buf, "FullScreen") == 0) {
            fullScreen = atoi(value.buf);
          } else if (stricmp(name.buf, "ViewOnly") == 0) {
            sendPtrEvents = sendKeyEvents = !atoi(value.buf);
          } else if (stricmp(name.buf, "DisableClipboard") == 0) {
            clientCutText = serverCutText = !atoi(value.buf);
          } else if (stricmp(name.buf, "AutoScaling") == 0) {
            autoScaling = atoi(value.buf);
          } else if (stricmp(name.buf, "Scale") == 0) {
            scale = atoi(value.buf);
          }
        }
      }
    }
    fclose(f); f=0;

    // Process the Host and Port
    if (hostTmp.buf) {
      int hostLen = strlen(hostTmp.buf) + 2 + 17;
      host.replaceBuf(new char[hostLen]);
      strCopy(host.buf, hostTmp.buf, hostLen);
      if (portTmp) {
        strncat(host.buf, "::", hostLen-1);
        char tmp[16];
        sprintf(tmp, "%d", portTmp);
        strncat(host.buf, tmp, hostLen-1);
      }
    }

    // If AutoSelect is enabled then override the preferred encoding
    if (autoSelect)
      preferredEncoding = encodingZRLE;

    setConfigFileName(filename);
  } catch (rdr::Exception&) {
    if (f) fclose(f);
    throw;
  }
}

void CConnOptions::writeToFile(const char* filename) {
  FILE* f = fopen(filename, "w");
  if (!f)
    throw rdr::Exception("Failed to write configuration file");

  try {
    // - Split server into host and port and save
    fprintf(f, "[Connection]\n");

    fprintf(f, "Host=%s\n", host.buf);
    if (userName.buf)
      fprintf(f, "UserName=%s\n", userName.buf);
    if (password.buf) {
      // - Warn the user before saving the password
      if (MsgBox(0, _T("Do you want to include the VNC Password in this configuration file?\n")
                    _T("Storing the password is more convenient but poses a security risk."),
                    MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) == IDYES) {
        ObfuscatedPasswd obfPwd(password);
        CharArray obfuscatedHex(rdr::HexOutStream::binToHexStr(obfPwd.buf, obfPwd.length));
        fprintf(f, "Password=%s\n", obfuscatedHex.buf);
      }
    }

    // - Save the other options
    fprintf(f, "[Options]\n");

    fprintf(f, "UseLocalCursor=%d\n", (int)useLocalCursor);
    fprintf(f, "UseDesktopResize=%d\n", (int)useDesktopResize);
    fprintf(f, "FullScreen=%d\n", (int)fullScreen);
    fprintf(f, "FullColour=%d\n", (int)fullColour);
    fprintf(f, "LowColourLevel=%d\n", lowColourLevel);
    fprintf(f, "PreferredEncoding=%s\n", encodingName(preferredEncoding));
    fprintf(f, "AutoSelect=%d\n", (int)autoSelect);
    fprintf(f, "Shared=%d\n", (int)shared);
    fprintf(f, "SendPtrEvents=%d\n", (int)sendPtrEvents);
    fprintf(f, "SendKeyEvents=%d\n", (int)sendKeyEvents);
    fprintf(f, "SendCutText=%d\n", (int)clientCutText);
    fprintf(f, "AcceptCutText=%d\n", (int)serverCutText);
    fprintf(f, "DisableWinKeys=%d\n", (int)disableWinKeys);
    fprintf(f, "AcceptBell=%d\n", (int)acceptBell);
    fprintf(f, "Emulate3=%d\n", (int)emulate3);
    fprintf(f, "ShowToolbar=%d\n", (int)showToolbar);
    fprintf(f, "PointerEventInterval=%d\n", pointerEventInterval);
    if (monitor.buf)
      fprintf(f, "Monitor=%s\n", monitor.buf);
    fprintf(f, "MenuKey=%s\n", CharArray(menuKeyName()).buf);
    fprintf(f, "AutoReconnect=%d\n", (int)autoReconnect);
    fprintf(f, "CustomCompressLevel=%d\n", customCompressLevel);
    fprintf(f, "CompressLevel=%d\n", compressLevel);
    fprintf(f, "NoJPEG=%d\n", noJpeg);
    fprintf(f, "QualityLevel=%d\n", qualityLevel);
    fprintf(f, "AutoScaling=%d\n", (int)autoScaling);
    fprintf(f, "Scale=%d\n", scale);
    fclose(f); f=0;

    setConfigFileName(filename);
  } catch (rdr::Exception&) {
    if (f) fclose(f);
    throw;
  }
}


void CConnOptions::writeDefaults() {
  RegKey key;
  key.createKey(HKEY_CURRENT_USER, _T("Software\\TightVNC\\VNCviewer4"));
  key.setBool(_T("UseLocalCursor"), useLocalCursor);
  key.setBool(_T("UseDesktopResize"), useDesktopResize);
  key.setBool(_T("FullScreen"), fullScreen);
  key.setBool(_T("FullColour"), fullColour);
  key.setInt(_T("LowColourLevel"), lowColourLevel);
  key.setString(_T("PreferredEncoding"), TStr(encodingName(preferredEncoding)));
  key.setBool(_T("AutoSelect"), autoSelect);
  key.setBool(_T("Shared"), shared);
  key.setBool(_T("SendPointerEvents"), sendPtrEvents);
  key.setBool(_T("SendKeyEvents"), sendKeyEvents);
  key.setBool(_T("ClientCutText"), clientCutText);
  key.setBool(_T("ServerCutText"), serverCutText);
  key.setBool(_T("DisableWinKeys"), disableWinKeys);
  key.setBool(_T("Protocol3.3"), protocol3_3);
  key.setBool(_T("AcceptBell"), acceptBell);
  key.setBool(_T("ShowToolbar"), showToolbar);
  key.setBool(_T("Emulate3"), emulate3);
  key.setInt(_T("PointerEventInterval"), pointerEventInterval);
  if (monitor.buf)
    key.setString(_T("Monitor"), TStr(monitor.buf));
  key.setString(_T("MenuKey"), TCharArray(menuKeyName()).buf);
  key.setBool(_T("AutoReconnect"), autoReconnect);
  key.setInt(_T("CustomCompressLevel"), customCompressLevel);
  key.setInt(_T("CompressLevel"), compressLevel);
  key.setInt(_T("NoJPEG"), noJpeg);
  key.setInt(_T("QualityLevel"), qualityLevel);
  key.setBool(_T("AutoScaling"), autoScaling);
  key.setInt(_T("Scale"), scale);
}


void CConnOptions::setUserName(const char* user) {userName.replaceBuf(strDup(user));}
void CConnOptions::setPassword(const char* pwd) {password.replaceBuf(strDup(pwd));}
void CConnOptions::setConfigFileName(const char* cfn) {configFileName.replaceBuf(strDup(cfn));}
void CConnOptions::setHost(const char* h) {host.replaceBuf(strDup(h));}
void CConnOptions::setMonitor(const char* m) {monitor.replaceBuf(strDup(m));}

void CConnOptions::setMenuKey(const char* keyName) {
  if (!keyName[0]) {
    menuKey = 0;
  } else {
    menuKey = VK_F8;
    if (keyName[0] == 'F') {
      UINT fKey = atoi(&keyName[1]);
      if (fKey >= 1 && fKey <= 12)
        menuKey = fKey-1 + VK_F1;
    }
  }
}
char* CConnOptions::menuKeyName() {
  int fNum = (menuKey-VK_F1)+1;
  if (fNum<1 || fNum>12)
    return strDup("");
  CharArray menuKeyStr(4);
  sprintf(menuKeyStr.buf, "F%d", fNum);
  return menuKeyStr.takeBuf();
}


CConnOptions& CConnOptions::operator=(const CConnOptions& o) {
  useLocalCursor = o.useLocalCursor;
  useDesktopResize = o.useDesktopResize;
  fullScreen = o.fullScreen;
  fullColour = o.fullColour;
  lowColourLevel = o.lowColourLevel;
  preferredEncoding = o.preferredEncoding;
  autoSelect = o.autoSelect;
  shared = o.shared;
  sendPtrEvents = o.sendPtrEvents;
  sendKeyEvents = o.sendKeyEvents;
  clientCutText = o.clientCutText;
  serverCutText = o.serverCutText;
  disableWinKeys = o.disableWinKeys;
  emulate3 = o.emulate3;
  pointerEventInterval = o.pointerEventInterval;
  protocol3_3 = o.protocol3_3;
  acceptBell = o.acceptBell;
  showToolbar = o.showToolbar;
  setUserName(o.userName.buf);
  setPassword(o.password.buf);
  setConfigFileName(o.configFileName.buf);
  setHost(o.host.buf);
  setMonitor(o.monitor.buf);
  menuKey = o.menuKey;
  autoReconnect = o.autoReconnect;
  customCompressLevel = o.customCompressLevel;
  compressLevel = o.compressLevel;
  noJpeg = o.noJpeg;
  qualityLevel = o.qualityLevel;
  autoScaling = o.autoScaling;
  scale = o.scale;

  return *this;
}
