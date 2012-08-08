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

using namespace rfb;

static LogWriter vlog("Parameters");


IntParameter pointerEventInterval("PointerEventInterval",
                                  "Time in milliseconds to rate-limit"
                                  " successive pointer events", 0);
BoolParameter dotWhenNoCursor("DotWhenNoCursor",
                              "Show the dot cursor when the server sends an "
                              "invisible cursor", false);

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
                            "0 = Very Low (8 colors), 1 = Low (64 colors), "
                            "2 = Medium (256 colors)", 2);
AliasParameter lowColourLevelAlias("LowColourLevel", "Alias for LowColorLevel", &lowColourLevel);
StringParameter preferredEncoding("PreferredEncoding",
                                  "Preferred encoding to use (Tight, ZRLE, Hextile or"
                                  " Raw)", "Tight");
BoolParameter customCompressLevel("CustomCompressLevel",
                                  "Use custom compression level. "
                                  "Default if CompressLevel is specified.", false);
IntParameter compressLevel("CompressLevel",
                           "Use specified compression level 0 = Low, 6 = High",
                           2);
BoolParameter noJpeg("NoJPEG",
                     "Disable lossy JPEG compression in Tight encoding.",
                     false);
IntParameter qualityLevel("QualityLevel",
                          "JPEG quality level. 0 = Low, 9 = High",
                          8);

BoolParameter maximize("Maximize", "Maximize viewer window", false);
#ifdef HAVE_FLTK_FULLSCREEN
BoolParameter fullScreen("FullScreen", "Full screen mode", false);
#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
BoolParameter fullScreenAllMonitors("FullScreenAllMonitors",
                                    "Enable full screen over all monitors",
                                    true);
#endif // HAVE_FLTK_FULLSCREEN_SCREENS
#endif // HAVE_FLTK_FULLSCREEN
StringParameter desktopSize("DesktopSize",
                            "Reconfigure desktop size on the server on "
                            "connect (if possible)", "");
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
BoolParameter sendPrimary("SendPrimary",
                          "Send the primary selection and cut buffer to the "
                          "server as well as the clipboard selection",
                          true);

StringParameter menuKey("MenuKey", "The key which brings up the popup menu",
                        "F8");

BoolParameter fullscreenSystemKeys("FullscreenSystemKeys",
                                   "Pass special keys (like Alt+Tab) directly "
                                   "to the server when in full screen mode.",
                                   true);

const char* IDENTIFIER_STRING = "TigerVNC Configuration file Version 1.0";

VoidParameter* parameterArray[] = {
#ifdef HAVE_GNUTLS
  &CSecurityTLS::x509ca,
  &CSecurityTLS::x509crl,
#endif // HAVE_GNUTLS
  &SecurityClient::secTypes,
  &dotWhenNoCursor,
  &autoSelect,
  &fullColour,
  &lowColourLevel,
  &preferredEncoding,
  &customCompressLevel,
  &compressLevel,
  &noJpeg,
  &qualityLevel,
#ifdef HAVE_FLTK_FULLSCREEN
  &fullScreen,
#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
  &fullScreenAllMonitors,
#endif // HAVE_FLTK_FULLSCREEN_SCREENS
#endif // HAVE_FLTK_FULLSCREEN
  &desktopSize,
  &remoteResize,
  &viewOnly,
  &shared,
  &acceptClipboard,
  &sendClipboard,
  &sendPrimary,
  &menuKey,
  &fullscreenSystemKeys
};

// Encoding Table
static struct {
  const char first;
  const char second;
} replaceMap[] = {'\n', 'n',
                  '\r', 'r'};

bool encodeValue(const char* val, char* dest, size_t destSize) {

  bool normalCharacter = true;
  size_t pos = 0;

  for (int i = 0; (val[i] != '\0') && (i < (destSize - 1)); i++) {
    
    // Check for sequences which will need encoding
    if (val[i] == '\\') {

      strncpy(dest+pos, "\\\\", 2);
      pos++;
      if (pos >= destSize) {
	vlog.error("Encoding backslash: The size of the buffer dest is to small, "
		   "it needs to be more than %d bytes bigger.", (destSize - 1 - i));
	return false;
      }

    } else {

      for (int j = 0; j < sizeof(replaceMap)/sizeof(replaceMap[0]); j++)

	if (val[i] == replaceMap[j].first) {
	  dest[pos] = '\\';
	  pos++;
	  if (pos >= destSize) {
	    vlog.error("Encoding escape sequence: The size of the buffer dest is to small, "
		       "it needs to be more than %d bytes bigger.", (destSize - 1 - i));
	    return false;
	  }

	  dest[pos] = replaceMap[j].second;
	  normalCharacter = false;
	  break;
	}

      if (normalCharacter) {
	dest[pos] = val[i];
      }
    }
    normalCharacter = true; // Reset for next loop

    pos++;
    if (pos >= destSize) {
      vlog.error("Encoding normal character: The size of the buffer dest is to small, "
		 "it needs to be more than %d bytes bigger.", (destSize - 1 - i));
      return false;
    }

  }

  dest[pos] = '\0';
  return true;
}


bool decodeValue(const char* val, char* dest, size_t destSize) {

  size_t pos = 0;
  bool escapedCharacter = false;
  
  for (int i = 0; (val[i] != '\0') && (i < (destSize - 1)); i++) {
    
    // Check for escape sequences
    if (val[i] == '\\') {
      
      for (int j = 0; j < sizeof(replaceMap)/sizeof(replaceMap[0]); j++) {
	if (val[i+1] == replaceMap[j].second) {
	  dest[pos] = replaceMap[j].first;
	  escapedCharacter = true;
	  pos--;
	  break;
	}
      }

      if (!escapedCharacter) {
	if (val[i+1] == '\\') {
	  dest[pos] = val[i];
	  i++;
	} else {
	  vlog.error("Unknown escape sequence at character %d", i);
	  return false;
	}
      }

    } else {
      dest[pos] = val[i];
    }

    escapedCharacter = false; // Reset for next loop
    pos++;
    if (pos >= destSize) {
      vlog.error("Decoding: The size of the buffer dest is to small, "
		 "it needs to be 1 byte bigger.");
      return false;
    }
  }
  
  dest[pos] = '\0';
  return true;
}


#ifdef _WIN32
void setKeyString(const char *_name, const char *_value, HKEY* hKey) {
  
  const DWORD buffersize = 256;

  wchar_t name[buffersize];
  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize) {
    vlog.error("Could not convert the parameter-name %s to wchar_t* when "
	       "writing to the Registry, the buffersize is to small.", _name);
    return;
  }

  char encodingBuffer[buffersize];
  if (!encodeValue(_value, encodingBuffer, buffersize)) {
    vlog.error("Could not encode the parameter-value %s when "
	       "writing to the Registry.", _value);
    return;
  }

  wchar_t value[buffersize];
  size = fl_utf8towc(encodingBuffer, strlen(encodingBuffer)+1, value, buffersize);
  if (size >= buffersize) {
    vlog.error("Could not convert the parameter-value %s to wchar_t* when "
	       "writing to the Registry, the buffersize is to small.", _value);
    return;
  }

  LONG res = RegSetValueExW(*hKey, name, 0, REG_SZ, (BYTE*)&value, (wcslen(value)+1)*2);
  if (res != ERROR_SUCCESS) {
    vlog.error("Error(%d) writing %s(REG_SZ) to Registry.", res, _value);
    return;
  }
}


void setKeyInt(const char *_name, const int _value, HKEY* hKey) {

  const DWORD buffersize = 256;
  wchar_t name[buffersize];
  DWORD value = _value;

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize) {
    vlog.error("Could not convert the parameter-name %s to wchar_t* when "
	       "writing to the Registry, the buffersize is to small.", _name);
    return;
  }
  
  LONG res = RegSetValueExW(*hKey, name, 0, REG_DWORD, (BYTE*)&value, sizeof(DWORD));
  if (res != ERROR_SUCCESS) {
    vlog.error("Error(%d) writing %d(REG_DWORD) to Registry.", res, _value);
    return;
  }
}


bool getKeyString(const char* _name, char* dest, size_t destSize, HKEY* hKey) {
  
  DWORD buffersize = 256;
  WCHAR value[destSize];
  wchar_t name[buffersize];

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize) {
    vlog.error("Could not convert the parameter-name %s to wchar_t* when "
	       "reading from the Registry, the buffersize is to small.", _name);
    return false;
  }

  LONG res = RegQueryValueExW(*hKey, name, 0, NULL, (LPBYTE)value, &buffersize);
  if (res != ERROR_SUCCESS){
    if (res == ERROR_FILE_NOT_FOUND) {
      // The value does not exist, defaults will be used.
    } else {
      vlog.error("Error(%d) reading %s from Registry.", res, _name);
    }
    return false;
  }
  
  char utf8val[destSize];
  size = fl_utf8fromwc(utf8val, sizeof(utf8val), value, wcslen(value)+1);
  if (size >= sizeof(utf8val)) {
    vlog.error("Could not convert the parameter-value for %s to utf8 char* "
	       "when reading from the Registry, the buffer dest is to small.",
	       _name);
    return false;
  }
  const char *ret = utf8val;
  
  if(decodeValue(ret, dest, destSize))
    return true;
  else 
    return false;
}


bool getKeyInt(const char* _name, int* dest, HKEY* hKey) {
  
  const DWORD buffersize = 256;
  DWORD dwordsize = sizeof(DWORD);
  DWORD value = 0;
  wchar_t name[buffersize];

  unsigned size = fl_utf8towc(_name, strlen(_name)+1, name, buffersize);
  if (size >= buffersize) {
    vlog.error("Could not convert the parameter-name %s to wchar_t* when "
	       "reading from the Registry, the buffersize is to small.", _name);
    return false;
  }

  LONG res = RegQueryValueExW(*hKey, name, 0, NULL, (LPBYTE)&value, &dwordsize);
  if (res != ERROR_SUCCESS){
    if (res == ERROR_FILE_NOT_FOUND) {
      // The value does not exist, defaults will be used.
    } else {
      vlog.error("Error(%d) reading %s from Registry.", res, _name);
    }
    return false;
  }

  *dest = (int)value;
  return true;
}


void saveToReg(const char* servername) {
  
  HKEY hKey;
    
  LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TigerVNC\\vncviewer", 0, NULL, 
			     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
  if (res != ERROR_SUCCESS) {
    vlog.error("Error(%d) creating key: Software\\TigerVNC\\vncviewer", res);
    return;
  }

  setKeyString("ServerName", servername, &hKey);

  for (int i = 0; i < sizeof(parameterArray)/sizeof(VoidParameter*); i++) {
    if (dynamic_cast<StringParameter*>(parameterArray[i]) != NULL) {
      setKeyString(parameterArray[i]->getName(), *(StringParameter*)parameterArray[i], &hKey);
    } else if (dynamic_cast<IntParameter*>(parameterArray[i]) != NULL) {
      setKeyInt(parameterArray[i]->getName(), (int)*(IntParameter*)parameterArray[i], &hKey);
    } else if (dynamic_cast<BoolParameter*>(parameterArray[i]) != NULL) {
      setKeyInt(parameterArray[i]->getName(), (int)*(BoolParameter*)parameterArray[i], &hKey);
    } else {      
      vlog.info("The parameterArray contains a object of a invalid type at line %d.", i);
    }
  }

  res = RegCloseKey(hKey);
  if (res != ERROR_SUCCESS) {
    vlog.error("Error(%d) closing key: Software\\TigerVNC\\vncviewer", res);
  }
}


char* loadFromReg() {

  HKEY hKey;

  LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TigerVNC\\vncviewer"
			   , 0, KEY_READ, &hKey);
  if (res != ERROR_SUCCESS) {
    if (res == ERROR_FILE_NOT_FOUND) {
      // The key does not exist, defaults will be used.
    } else {
      vlog.error("Error(%d) opening key: Software\\TigerVNC\\vncviewer", res);
    }
    return NULL;
  }

  const size_t buffersize = 256;
  static char servername[buffersize];

  char servernameBuffer[buffersize];
  if (getKeyString("ServerName", servernameBuffer, buffersize, &hKey))
    snprintf(servername, buffersize, "%s", servernameBuffer);
  
  int intValue = 0;
  char stringValue[buffersize];
  
  for (int i = 0; i < sizeof(parameterArray)/sizeof(VoidParameter*); i++) {
    if (dynamic_cast<StringParameter*>(parameterArray[i]) != NULL) {
      if (getKeyString(parameterArray[i]->getName(), stringValue, buffersize, &hKey))
	parameterArray[i]->setParam(stringValue);
    } else if (dynamic_cast<IntParameter*>(parameterArray[i]) != NULL) {
      if (getKeyInt(parameterArray[i]->getName(), &intValue, &hKey))
	((IntParameter*)parameterArray[i])->setParam(intValue);
    } else if (dynamic_cast<BoolParameter*>(parameterArray[i]) != NULL) {
      if (getKeyInt(parameterArray[i]->getName(), &intValue, &hKey))
	((BoolParameter*)parameterArray[i])->setParam(intValue);
    } else {      
      vlog.info("The parameterArray contains a object of a invalid type at line %d.", i);
    }
  }

  res = RegCloseKey(hKey);
  if (res != ERROR_SUCCESS){
    vlog.error("Error(%d) closing key:  Software\\TigerVNC\\vncviewer", res);
  }
  
  return servername;
}
#endif // _WIN32


void saveViewerParameters(const char *filename, const char *servername) {

  const size_t buffersize = 256;
  char filepath[PATH_MAX];
  char write_error[buffersize*2];
  char encodingBuffer[buffersize];

  // Write to the registry or a predefined file if no filename was specified.
  if(filename == NULL) {

#ifdef _WIN32
    saveToReg(servername);
    return;
#endif
    
    char* homeDir = NULL;
    if (getvnchomedir(&homeDir) == -1) {
      vlog.error("Failed to write configuration file, "
		 "can't obtain home directory path.");
      return;
    }

    snprintf(filepath, sizeof(filepath), "%sdefault.tigervnc", homeDir);
  } else {
    snprintf(filepath, sizeof(filepath), "%s", filename);
  }

  /* Write parameters to file */
  FILE* f = fopen(filepath, "w+");
  if (!f) {
    snprintf(write_error, sizeof(filepath), "Failed to write configuration file, "
	     "can't open %s", filepath);
    throw Exception(write_error);
  }
  
  fprintf(f, "%s\r\n", IDENTIFIER_STRING);
  fprintf(f, "\r\n");

  if (encodeValue(servername, encodingBuffer, buffersize))  
    fprintf(f, "ServerName=%s\n", encodingBuffer);
  
  for (int i = 0; i < sizeof(parameterArray)/sizeof(VoidParameter*); i++) {
    if (dynamic_cast<StringParameter*>(parameterArray[i]) != NULL) {
      if (encodeValue(*(StringParameter*)parameterArray[i], encodingBuffer, buffersize))
	fprintf(f, "%s=%s\n", ((StringParameter*)parameterArray[i])->getName(), encodingBuffer);
    } else if (dynamic_cast<IntParameter*>(parameterArray[i]) != NULL) {
      fprintf(f, "%s=%d\n", ((IntParameter*)parameterArray[i])->getName(), (int)*(IntParameter*)parameterArray[i]);
    } else if (dynamic_cast<BoolParameter*>(parameterArray[i]) != NULL) {
      fprintf(f, "%s=%d\n", ((BoolParameter*)parameterArray[i])->getName(), (int)*(BoolParameter*)parameterArray[i]);
    } else {      
      vlog.info("The parameterArray contains a object of a invalid type at line %d.", i);
    }
  }
  fclose(f);
}


char* loadViewerParameters(const char *filename) {

  const size_t buffersize = 256;
  char filepath[PATH_MAX];
  char readError[buffersize*2];
  char line[buffersize];
  char decodingBuffer[buffersize];
  char decodedValue[buffersize];
  static char servername[sizeof(line)];

  // Load from the registry or a predefined file if no filename was specified.
  if(filename == NULL) {

#ifdef _WIN32
    return loadFromReg();
#endif

    char* homeDir = NULL;
    if (getvnchomedir(&homeDir) == -1)
      throw Exception("Failed to read configuration file, " 
			   "can't obtain home directory path.");

    snprintf(filepath, sizeof(filepath), "%sdefault.tigervnc", homeDir);
  } else {
    snprintf(filepath, sizeof(filepath), "%s", filename);
  }

  /* Read parameters from file */
  FILE* f = fopen(filepath, "r");
  if (!f) {
    if (!filename)
      return NULL; // Use defaults.
    snprintf(readError, sizeof(readError), "Failed to read configuration file, "
	     "can't open %s", filepath);
    throw Exception(readError);
  }
  
  int lineNr = 0;
  while (!feof(f)) {

    // Read the next line
    lineNr++;
    if (!fgets(line, sizeof(line), f)) {
      if (line[sizeof(line) -1] != '\0') {
	vlog.error("Could not read the line(%d) in the configuration file,"
		   "the buffersize is to small.", lineNr);
	return NULL;
      }
      if (feof(f))
	break;

      snprintf(readError, sizeof(readError), "Failed to read line %d in file %s", 
	       lineNr, filepath);
      throw Exception(readError);
    }
    
    // Make sure that the first line of the file has the file identifier string
    if(lineNr == 1) {
      if(strncmp(line, IDENTIFIER_STRING, strlen(IDENTIFIER_STRING)) == 0) {
	continue;
      } else {
	snprintf(readError, sizeof(readError), "Line 1 in file %s\n"
		 "must contain the TigerVNC configurationfile identifier string:\n"
		 "\"%s\"", filepath, IDENTIFIER_STRING);
	throw Exception(readError);
      }
    }
    
    // Skip empty lines and comments
    if ((line[0] == '\n') || (line[0] == '#') || (line[0] == '\r'))
      continue;

    int len = strlen(line);
    if (line[len-1] == '\n') {
      line[len-1] = '\0';
      len--;
    }

    // Find the parameter value
    char *value = strchr(line, '=');
    if (value == NULL) {
      vlog.info("Bad Name/Value pair on line: %d in file: %s", 
		lineNr, filepath);
      continue;
    }
    *value = '\0'; // line only contains the parameter name below.
    value++;
    
    bool invalidParameterName = true; // Will be set to false below if 
                                      // the line contains a valid name.

    if (strcasecmp(line, "ServerName") == 0) {

      if(!decodeValue(value, decodingBuffer, sizeof(decodingBuffer))) {
	vlog.info("The value of the parameter %s on line %d in file %s is invalid.",
		  line, lineNr, filepath);
	continue;
      }
      snprintf(servername, sizeof(decodingBuffer), "%s", decodingBuffer);
      invalidParameterName = false;

    } else {
    
      // Find and set the correct parameter
      for (int i = 0; i < sizeof(parameterArray)/sizeof(VoidParameter*); i++) {

	if (dynamic_cast<StringParameter*>(parameterArray[i]) != NULL) {
	  if (strcasecmp(line, ((StringParameter*)parameterArray[i])->getName()) == 0) {

	    if(!decodeValue(value, decodingBuffer, sizeof(decodingBuffer))) {
	      vlog.info("The value of the parameter %s on line %d in file %s is invalid.",
			line, lineNr, filepath);
	      continue;
	    }
	    ((StringParameter*)parameterArray[i])->setParam(decodingBuffer);
	    invalidParameterName = false;
	  }

	} else if (dynamic_cast<IntParameter*>(parameterArray[i]) != NULL) {
	  if (strcasecmp(line, ((IntParameter*)parameterArray[i])->getName()) == 0) {
	    ((IntParameter*)parameterArray[i])->setParam(atoi(value));
	    invalidParameterName = false;
	  }
	
	} else if (dynamic_cast<BoolParameter*>(parameterArray[i]) != NULL) {
	  if (strcasecmp(line, ((BoolParameter*)parameterArray[i])->getName()) == 0) {
	    ((BoolParameter*)parameterArray[i])->setParam(atoi(value));
	    invalidParameterName = false;
	  }

	} else {      
	  vlog.info("The parameterArray contains a object of a invalid type at line %d.", lineNr);
	}
      }
    }

    if (invalidParameterName)
      vlog.info("Invalid parameter name on line: %d in file: %s", 
		lineNr, filepath);
  }
  fclose(f); f=0;
  
  return servername;
}
