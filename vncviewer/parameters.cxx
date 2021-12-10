/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2012 Samuel Mannehed <samuel@cendio.se> for Cendio AB
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

#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include "parameters.h"

#include <os/os.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/SecurityClient.h>

#include <FL/fl_utf8.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "i18n.h"

using namespace rfb;
using namespace std;

static LogWriter vlog("Parameters");


IntParameter pointerEventInterval("PointerEventInterval",
                                  "Time in milliseconds to rate-limit"
                                  " successive pointer events", 17);
BoolParameter emulateMiddleButton("EmulateMiddleButton",
                                  "Emulate middle mouse button by pressing "
                                  "left and right mouse buttons simultaneously",
                                  false);
BoolParameter dotWhenNoCursor("DotWhenNoCursor",
                              "Show the dot cursor when the server sends an "
                              "invisible cursor", false);

BoolParameter alertOnFatalError("AlertOnFatalError",
                                "Give a dialog on connection problems rather "
                                "than exiting immediately", true);

BoolParameter reconnectOnError("ReconnectOnError",
                               "Give a dialog on connection problems rather "
                               "than exiting immediately and ask for a reconnect.", true);

StringParameter passwordFile("PasswordFile",
                             "Password file for VNC authentication", "");
AliasParameter passwd("passwd", "Alias for PasswordFile", &passwordFile);

BoolParameter autoSelect("AutoSelect",
                         "Auto select pixel format and encoding. "
                         "Default if PreferredEncoding and FullColor are not specified.", 
                         true);
BoolParameter fullColour("FullColor",
                         "Use full color", true);
AliasParameter fullColourAlias("FullColour", "Alias for FullColor", &fullColour);
IntParameter lowColourLevel("LowColorLevel",
                            "Color level to use on slow connections. "
                            "0 = Very Low, 1 = Low, 2 = Medium", 2);
AliasParameter lowColourLevelAlias("LowColourLevel", "Alias for LowColorLevel", &lowColourLevel);
StringParameter preferredEncoding("PreferredEncoding",
                                  "Preferred encoding to use (Tight, ZRLE, Hextile or"
                                  " Raw)", "Tight");
BoolParameter customCompressLevel("CustomCompressLevel",
                                  "Use custom compression level. "
                                  "Default if CompressLevel is specified.", false);
IntParameter compressLevel("CompressLevel",
                           "Use specified compression level 0 = Low, 9 = High",
                           2);
BoolParameter noJpeg("NoJPEG",
                     "Disable lossy JPEG compression in Tight encoding.",
                     false);
IntParameter qualityLevel("QualityLevel",
                          "JPEG quality level. 0 = Low, 9 = High",
                          8);

BoolParameter maximize("Maximize", "Maximize viewer window", false);
BoolParameter fullScreen("FullScreen", "Enable full screen", false);
StringParameter fullScreenMode("FullScreenMode", "Specify which monitors to use when in full screen. "
                                                 "Should be either Current, Selected or All",
                                                 "Current");
BoolParameter fullScreenAllMonitors("FullScreenAllMonitors",
                                    "[DEPRECATED] Enable full screen over all monitors",
                                    false);
MonitorIndicesParameter fullScreenSelectedMonitors("FullScreenSelectedMonitors",
                                         "Use the given list of monitors in full screen"
                                         " when -FullScreenMode=Selected.",
                                         "1");
StringParameter desktopSize("DesktopSize",
                            "Reconfigure desktop size on the server on "
                            "connect (if possible)", "");
StringParameter geometry("geometry",
                         "Specify size and position of viewer window", "");

BoolParameter listenMode("listen", "Listen for connections from VNC servers", false);

BoolParameter remoteResize("RemoteResize",
                           "Dynamically resize the remote desktop size as "
                           "the size of the local client window changes. "
                           "(Does not work with all servers)", true);

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
#if !defined(WIN32) && !defined(__APPLE__)
BoolParameter setPrimary("SetPrimary",
                         "Set the primary selection as well as the "
                         "clipboard selection", true);
BoolParameter sendPrimary("SendPrimary",
                          "Send the primary selection to the "
                          "server as well as the clipboard selection",
                          true);
StringParameter display("display",
			"Specifies the X display on which the VNC viewer window should appear.",
			"");
#endif

StringParameter menuKey("MenuKey", "The key which brings up the popup menu",
                        "F8");

BoolParameter fullscreenSystemKeys("FullscreenSystemKeys",
                                   "Pass special keys (like Alt+Tab) directly "
                                   "to the server when in full-screen mode.",
                                   true);

#ifndef WIN32
StringParameter via("via", "Gateway to tunnel via", "");
#endif

static const char* IDENTIFIER_STRING = "TigerVNC Configuration file Version 1.0";

/*
 * We only save the sub set of parameters that can be modified from
 * the graphical user interface
 */
static VoidParameter* parameterArray[] = {
  /* Security */
#ifdef HAVE_GNUTLS
  &CSecurityTLS::X509CA,
  &CSecurityTLS::X509CRL,
#endif // HAVE_GNUTLS
  &SecurityClient::secTypes,
  /* Misc. */
  &reconnectOnError,
  &shared,
  /* Compression */
  &autoSelect,
  &fullColour,
  &lowColourLevel,
  &preferredEncoding,
  &customCompressLevel,
  &compressLevel,
  &noJpeg,
  &qualityLevel,
  /* Display */
  &fullScreen,
  &fullScreenMode,
  &fullScreenSelectedMonitors,
  /* Input */
  &viewOnly,
  &emulateMiddleButton,
  &dotWhenNoCursor,
  &acceptClipboard,
  &sendClipboard,
#if !defined(WIN32) && !defined(__APPLE__)
  &sendPrimary,
  &setPrimary,
#endif
  &menuKey,
  &fullscreenSystemKeys
};

static VoidParameter* readOnlyParameterArray[] = {
  &fullScreenAllMonitors
};

// Encoding Table
static struct {
  const char first;
  const char second;
} replaceMap[] = { { '\n', 'n' },
                   { '\r', 'r' },
                   { '\\', '\\' } };

static bool encodeValue(const char* val, char* dest, size_t destSize) {

  size_t pos = 0;

  for (size_t i = 0; (val[i] != '\0') && (i < (destSize - 1)); i++) {
    bool normalCharacter;
    
    // Check for sequences which will need encoding
    normalCharacter = true;
    for (size_t j = 0; j < sizeof(replaceMap)/sizeof(replaceMap[0]); j++) {

      if (val[i] == replaceMap[j].first) {
        dest[pos] = '\\';
        pos++;
        if (pos >= destSize)
          return false;

        dest[pos] = replaceMap[j].second;
        normalCharacter = false;
        break;
      }

      if (normalCharacter) {
        dest[pos] = val[i];
      }
    }

    pos++;
    if (pos >= destSize)
      return false;
  }

  dest[pos] = '\0';
  return true;
}


static bool decodeValue(const char* val, char* dest, size_t destSize) {

  size_t pos = 0;
  
  for (size_t i = 0; (val[i] != '\0') && (i < (destSize - 1)); i++) {
    
    // Check for escape sequences
    if (val[i] == '\\') {
      bool escapedCharacter;
      
      escapedCharacter = false;
      for (size_t j = 0; j < sizeof(replaceMap)/sizeof(replaceMap[0]); j++) {
        if (val[i+1] == replaceMap[j].second) {
          dest[pos] = replaceMap[j].first;
          escapedCharacter = true;
          i++;
          break;
        }
      }

      if (!escapedCharacter)
        return false;

    } else {
      dest[pos] = val[i];
    }

    pos++;
    if (pos >= destSize) {
      return false;
    }
  }
  
  dest[pos] = '\0';
  return true;
}


#ifdef _WIN32
static void setKeyString(const char *_name, const char *_value, HKEY* hKey) {
  
  const DWORD buffersize = 256;

  wchar_t name[buffersize];
  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize)
    throw Exception(_("The name of the parameter is too large"));

  char encodingBuffer[buffersize];
  if (!encodeValue(_value, encodingBuffer, buffersize))
    throw Exception(_("The parameter is too large"));

  wchar_t value[buffersize];
  size = fl_utf8towc(encodingBuffer, strlen(encodingBuffer)+1, value, buffersize);
  if (size >= buffersize)
    throw Exception(_("The parameter is too large"));

  LONG res = RegSetValueExW(*hKey, name, 0, REG_SZ, (BYTE*)&value, (wcslen(value)+1)*2);
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException("RegSetValueExW", res);
}


static void setKeyInt(const char *_name, const int _value, HKEY* hKey) {

  const DWORD buffersize = 256;
  wchar_t name[buffersize];
  DWORD value = _value;

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize)
    throw Exception(_("The name of the parameter is too large"));

  LONG res = RegSetValueExW(*hKey, name, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException("RegSetValueExW", res);
}


static bool getKeyString(const char* _name, char* dest, size_t destSize, HKEY* hKey) {
  
  const DWORD buffersize = 256;
  wchar_t name[buffersize];
  WCHAR* value;
  DWORD valuesize;

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize)
    throw Exception(_("The name of the parameter is too large"));

  value = new WCHAR[destSize];
  valuesize = destSize;
  LONG res = RegQueryValueExW(*hKey, name, 0, NULL, (LPBYTE)value, &valuesize);
  if (res != ERROR_SUCCESS){
    delete [] value;
    if (res != ERROR_FILE_NOT_FOUND)
      throw rdr::SystemException("RegQueryValueExW", res);
    // The value does not exist, defaults will be used.
    return false;
  }

  char* utf8val = new char[destSize];
  size = fl_utf8fromwc(utf8val, destSize, value, wcslen(value)+1);
  delete [] value;
  if (size >= destSize) {
    delete [] utf8val;
    throw Exception(_("The parameter is too large"));
  }

  bool ret = decodeValue(utf8val, dest, destSize);
  delete [] utf8val;

  if (!ret)
    throw Exception(_("Invalid format or too large value"));

  return true;
}


static bool getKeyInt(const char* _name, int* dest, HKEY* hKey) {
  
  const DWORD buffersize = 256;
  DWORD dwordsize = sizeof(DWORD);
  DWORD value = 0;
  wchar_t name[buffersize];

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize)
    throw Exception(_("The name of the parameter is too large"));

  LONG res = RegQueryValueExW(*hKey, name, 0, NULL, (LPBYTE)&value, &dwordsize);
  if (res != ERROR_SUCCESS){
    if (res != ERROR_FILE_NOT_FOUND)
      throw rdr::SystemException("RegQueryValueExW", res);
    // The value does not exist, defaults will be used.
    return false;
  }

  *dest = (int)value;
  return true;
}

static void removeValue(const char* _name, HKEY* hKey) {
  const DWORD buffersize = 256;
  wchar_t name[buffersize];

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize)
    throw Exception(_("The name of the parameter is too large"));

  LONG res = RegDeleteValueW(*hKey, name);
  if (res != ERROR_SUCCESS) {
    if (res != ERROR_FILE_NOT_FOUND)
      throw rdr::SystemException("RegDeleteValueW", res);
    // The value does not exist, no need to remove it.
    return;
  }
}

void saveHistoryToRegKey(const vector<string>& serverHistory) {
  HKEY hKey;
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER,
                             L"Software\\TigerVNC\\vncviewer\\history", 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                             &hKey, NULL);

  if (res != ERROR_SUCCESS)
    throw rdr::SystemException(_("Failed to create registry key"), res);

  size_t index = 0;
  assert(SERVER_HISTORY_SIZE < 100);
  char indexString[3];

  try {
    while(index < serverHistory.size() && index <= SERVER_HISTORY_SIZE) {
      snprintf(indexString, 3, "%d", index);
      setKeyString(indexString, serverHistory[index].c_str(), &hKey);
      index++;
    }
  } catch (Exception& e) {
    RegCloseKey(hKey);
    throw;
  }

  res = RegCloseKey(hKey);
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException(_("Failed to close registry key"), res);
}

static void saveToReg(const char* servername) {
  
  HKEY hKey;
    
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER,
                             L"Software\\TigerVNC\\vncviewer", 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                             &hKey, NULL);
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException(_("Failed to create registry key"), res);

  try {
    setKeyString("ServerName", servername, &hKey);
  } catch (Exception& e) {
    RegCloseKey(hKey);
    throw Exception(_("Failed to save \"%s\": %s"),
                    "ServerName", e.str());
  }

  for (size_t i = 0; i < sizeof(parameterArray)/sizeof(VoidParameter*); i++) {
    try {
      if (dynamic_cast<StringParameter*>(parameterArray[i]) != NULL) {
        setKeyString(parameterArray[i]->getName(), *(StringParameter*)parameterArray[i], &hKey);
      } else if (dynamic_cast<IntParameter*>(parameterArray[i]) != NULL) {
        setKeyInt(parameterArray[i]->getName(), (int)*(IntParameter*)parameterArray[i], &hKey);
      } else if (dynamic_cast<BoolParameter*>(parameterArray[i]) != NULL) {
        setKeyInt(parameterArray[i]->getName(), (int)*(BoolParameter*)parameterArray[i], &hKey);
      } else {
        throw Exception(_("Unknown parameter type"));
      }
    } catch (Exception& e) {
      RegCloseKey(hKey);
      throw Exception(_("Failed to save \"%s\": %s"),
                      parameterArray[i]->getName(), e.str());
    }
  }

  // Remove read-only parameters to replicate the behaviour of Linux/macOS when they
  // store a config to disk. If the parameter hasn't been migrated at this point it
  // will be lost.
  for (size_t i = 0; i < sizeof(readOnlyParameterArray)/sizeof(VoidParameter*); i++) {
    try {
      removeValue(readOnlyParameterArray[i]->getName(), &hKey);
    } catch (Exception& e) {
      RegCloseKey(hKey);
      throw Exception(_("Failed to remove \"%s\": %s"),
                      readOnlyParameterArray[i]->getName(), e.str());
    }
  }

  res = RegCloseKey(hKey);
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException(_("Failed to close registry key"), res);
}

void loadHistoryFromRegKey(vector<string>& serverHistory) {
  HKEY hKey;

  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER,
                           L"Software\\TigerVNC\\vncviewer\\history", 0,
                           KEY_READ, &hKey);
  if (res != ERROR_SUCCESS) {
    if (res == ERROR_FILE_NOT_FOUND) {
      // The key does not exist, defaults will be used.
      return;
    }

    throw rdr::SystemException(_("Failed to open registry key"), res);
  }

  size_t index;
  const DWORD buffersize = 256;
  char indexString[3];

  for (index = 0;;index++) {
    snprintf(indexString, 3, "%d", index);
    char servernameBuffer[buffersize];

    try {
      if (!getKeyString(indexString, servernameBuffer,
                        buffersize, &hKey))
        break;
    } catch (Exception& e) {
      // Just ignore this entry and try the next one
      vlog.error(_("Failed to read server history entry %d: %s"),
                 (int)index, e.str());
      continue;
    }

    serverHistory.push_back(servernameBuffer);
  }

  res = RegCloseKey(hKey);
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException(_("Failed to close registry key"), res);
}

static void getParametersFromReg(VoidParameter* parameters[],
                                 size_t parameters_len, HKEY* hKey)
{
  const size_t buffersize = 256;
  int intValue = 0;
  char stringValue[buffersize];

  for (size_t i = 0; i < parameters_len/sizeof(VoidParameter*); i++) {
    try {
      if (dynamic_cast<StringParameter*>(parameters[i]) != NULL) {
        if (getKeyString(parameters[i]->getName(), stringValue, buffersize, hKey))
          parameters[i]->setParam(stringValue);
      } else if (dynamic_cast<IntParameter*>(parameters[i]) != NULL) {
        if (getKeyInt(parameters[i]->getName(), &intValue, hKey))
          ((IntParameter*)parameters[i])->setParam(intValue);
      } else if (dynamic_cast<BoolParameter*>(parameters[i]) != NULL) {
        if (getKeyInt(parameters[i]->getName(), &intValue, hKey))
          ((BoolParameter*)parameters[i])->setParam(intValue);
      } else {
        throw Exception(_("Unknown parameter type"));
      }
    } catch(Exception& e) {
      // Just ignore this entry and continue with the rest
      vlog.error(_("Failed to read parameter \"%s\": %s"),
                 parameters[i]->getName(), e.str());
    }
  }
}

static char* loadFromReg() {

  HKEY hKey;

  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER,
                           L"Software\\TigerVNC\\vncviewer", 0,
                           KEY_READ, &hKey);
  if (res != ERROR_SUCCESS) {
    if (res == ERROR_FILE_NOT_FOUND) {
      // The key does not exist, defaults will be used.
      return NULL;
    }

    throw rdr::SystemException(_("Failed to open registry key"), res);
  }

  const size_t buffersize = 256;
  static char servername[buffersize];

  char servernameBuffer[buffersize];
  try {
    if (getKeyString("ServerName", servernameBuffer, buffersize, &hKey))
      snprintf(servername, buffersize, "%s", servernameBuffer);
  } catch(Exception& e) {
    vlog.error(_("Failed to read parameter \"%s\": %s"),
               "ServerName", e.str());
    strcpy(servername, "");
  }

  getParametersFromReg(parameterArray, sizeof(parameterArray), &hKey);
  getParametersFromReg(readOnlyParameterArray,
                       sizeof(readOnlyParameterArray), &hKey);

  res = RegCloseKey(hKey);
  if (res != ERROR_SUCCESS)
    throw rdr::SystemException(_("Failed to close registry key"), res);

  return servername;
}
#endif // _WIN32


void saveViewerParameters(const char *filename, const char *servername) {

  const size_t buffersize = 256;
  char filepath[PATH_MAX];
  char encodingBuffer[buffersize];

  // Write to the registry or a predefined file if no filename was specified.
  if(filename == NULL) {

#ifdef _WIN32
    saveToReg(servername);
    return;
#endif
    
    char* homeDir = NULL;
    if (getvnchomedir(&homeDir) == -1)
      throw Exception(_("Could not obtain the home directory path"));

    snprintf(filepath, sizeof(filepath), "%sdefault.tigervnc", homeDir);
    delete[] homeDir;
  } else {
    snprintf(filepath, sizeof(filepath), "%s", filename);
  }

  /* Write parameters to file */
  FILE* f = fopen(filepath, "w+");
  if (!f)
    throw Exception(_("Could not open \"%s\": %s"),
                    filepath, strerror(errno));

  fprintf(f, "%s\n", IDENTIFIER_STRING);
  fprintf(f, "\n");

  if (!encodeValue(servername, encodingBuffer, buffersize)) {
    fclose(f);
    throw Exception(_("Failed to save \"%s\": %s"),
                    "ServerName", _("Could not encode parameter"));
  }
  fprintf(f, "ServerName=%s\n", encodingBuffer);

  for (size_t i = 0; i < sizeof(parameterArray)/sizeof(VoidParameter*); i++) {
    if (dynamic_cast<StringParameter*>(parameterArray[i]) != NULL) {
      if (!encodeValue(*(StringParameter*)parameterArray[i],
          encodingBuffer, buffersize)) {
        fclose(f);
        throw Exception(_("Failed to save \"%s\": %s"),
                        parameterArray[i]->getName(),
                        _("Could not encode parameter"));
      }
      fprintf(f, "%s=%s\n", ((StringParameter*)parameterArray[i])->getName(), encodingBuffer);
    } else if (dynamic_cast<IntParameter*>(parameterArray[i]) != NULL) {
      fprintf(f, "%s=%d\n", ((IntParameter*)parameterArray[i])->getName(), (int)*(IntParameter*)parameterArray[i]);
    } else if (dynamic_cast<BoolParameter*>(parameterArray[i]) != NULL) {
      fprintf(f, "%s=%d\n", ((BoolParameter*)parameterArray[i])->getName(), (int)*(BoolParameter*)parameterArray[i]);
    } else {      
      fclose(f);
      throw Exception(_("Failed to save \"%s\": %s"),
                      parameterArray[i]->getName(),
                      _("Unknown parameter type"));
    }
  }
  fclose(f);
}

static bool findAndSetViewerParameterFromValue(
  VoidParameter* parameters[], size_t parameters_len,
  char* value, char* line, char* filepath)
{
  const size_t buffersize = 256;
  char decodingBuffer[buffersize];

  // Find and set the correct parameter
  for (size_t i = 0; i < parameters_len/sizeof(VoidParameter*); i++) {

    if (dynamic_cast<StringParameter*>(parameters[i]) != NULL) {
      if (strcasecmp(line, ((StringParameter*)parameters[i])->getName()) == 0) {
        if(!decodeValue(value, decodingBuffer, sizeof(decodingBuffer)))
          throw Exception(_("Invalid format or too large value"));
        ((StringParameter*)parameters[i])->setParam(decodingBuffer);
        return false;
      }

    } else if (dynamic_cast<IntParameter*>(parameters[i]) != NULL) {
      if (strcasecmp(line, ((IntParameter*)parameters[i])->getName()) == 0) {
        ((IntParameter*)parameters[i])->setParam(atoi(value));
        return false;
      }

    } else if (dynamic_cast<BoolParameter*>(parameters[i]) != NULL) {
      if (strcasecmp(line, ((BoolParameter*)parameters[i])->getName()) == 0) {
        ((BoolParameter*)parameters[i])->setParam(atoi(value));
        return false;
      }

    } else {
      throw Exception(_("Unknown parameter type"));
    }
  }

  return true;
}

char* loadViewerParameters(const char *filename) {

  const size_t buffersize = 256;
  char filepath[PATH_MAX];
  char line[buffersize];
  char decodingBuffer[buffersize];
  static char servername[sizeof(line)];

  memset(servername, '\0', sizeof(servername));

  // Load from the registry or a predefined file if no filename was specified.
  if(filename == NULL) {

#ifdef _WIN32
    return loadFromReg();
#endif

    char* homeDir = NULL;
    if (getvnchomedir(&homeDir) == -1)
      throw Exception(_("Could not obtain the home directory path"));

    snprintf(filepath, sizeof(filepath), "%sdefault.tigervnc", homeDir);
    delete[] homeDir;
  } else {
    snprintf(filepath, sizeof(filepath), "%s", filename);
  }

  /* Read parameters from file */
  FILE* f = fopen(filepath, "r");
  if (!f) {
    if (!filename)
      return NULL; // Use defaults.
    throw Exception(_("Could not open \"%s\": %s"),
                    filepath, strerror(errno));
  }
  
  int lineNr = 0;
  while (!feof(f)) {

    // Read the next line
    lineNr++;
    if (!fgets(line, sizeof(line), f)) {
      if (feof(f))
        break;

      fclose(f);
      throw Exception(_("Failed to read line %d in file %s: %s"),
                      lineNr, filepath, strerror(errno));
    }

    if (strlen(line) == (sizeof(line) - 1)) {
      fclose(f);
      throw Exception(_("Failed to read line %d in file %s: %s"),
                      lineNr, filepath, _("Line too long"));
    }

    // Make sure that the first line of the file has the file identifier string
    if(lineNr == 1) {
      if(strncmp(line, IDENTIFIER_STRING, strlen(IDENTIFIER_STRING)) == 0)
        continue;

      fclose(f);
      throw Exception(_("Configuration file %s is in an invalid format"),
                      filepath);
    }
    
    // Skip empty lines and comments
    if ((line[0] == '\n') || (line[0] == '#') || (line[0] == '\r'))
      continue;

    int len = strlen(line);
    if (line[len-1] == '\n') {
      line[len-1] = '\0';
      len--;
    }
    if (line[len-1] == '\r') {
      line[len-1] = '\0';
      len--;
    }

    // Find the parameter value
    char *value = strchr(line, '=');
    if (value == NULL) {
      vlog.error(_("Failed to read line %d in file %s: %s"),
                 lineNr, filepath, _("Invalid format"));
      continue;
    }
    *value = '\0'; // line only contains the parameter name below.
    value++;
    
    bool invalidParameterName = true; // Will be set to false below if 
                                      // the line contains a valid name.

    try {
      if (strcasecmp(line, "ServerName") == 0) {

        if(!decodeValue(value, decodingBuffer, sizeof(decodingBuffer)))
          throw Exception(_("Invalid format or too large value"));
        snprintf(servername, sizeof(decodingBuffer), "%s", decodingBuffer);
        invalidParameterName = false;

      } else {
        invalidParameterName = findAndSetViewerParameterFromValue(parameterArray, sizeof(parameterArray),
                                                                  value, line, filepath);

        if (invalidParameterName) {
          invalidParameterName = findAndSetViewerParameterFromValue(readOnlyParameterArray, sizeof(readOnlyParameterArray),
                                                                    value, line, filepath);
        }
      }
    } catch(Exception& e) {
      // Just ignore this entry and continue with the rest
      vlog.error(_("Failed to read line %d in file %s: %s"),
                 lineNr, filepath, e.str());
      continue;
    }

    if (invalidParameterName)
      vlog.error(_("Failed to read line %d in file %s: %s"),
                 lineNr, filepath, _("Unknown parameter"));
  }
  fclose(f); f=0;
  
  return servername;
}
