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

#include <vncconfig/Legacy.h>

#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb_win32/CurrentUser.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("Legacy");


void LegacyPage::LoadPrefs()
      {
        // VNC 3.3.3R3 Preferences Algorithm, as described by vncProperties.cpp
        // - Load user-specific settings, based on logged-on user name,
        //   from HKLM/Software/ORL/WinVNC3/<user>.  If they don't exist,
        //   try again with username "Default".
        // - Load system-wide settings from HKLM/Software/ORL/WinVNC3.
        // - If AllowProperties is non-zero then load the user's own
        //   settings from HKCU/Software/ORL/WinVNC3.

        // Get the name of the current user
        std::string username;
        try {
          username = UserName();
        } catch (rdr::SystemException& e) {
          if (e.err != ERROR_NOT_LOGGED_ON)
            throw;
        }

        // Open and read the WinVNC3 registry key
        allowProperties = true;
        RegKey winvnc3;
        try {
          winvnc3.openKey(HKEY_LOCAL_MACHINE, "Software\\ORL\\WinVNC3");
          int debugMode = winvnc3.getInt("DebugMode", 0);
          const char* debugTarget = nullptr;
          if (debugMode & 2) debugTarget = "file";
          if (debugMode & 4) debugTarget = "stderr";
          if (debugTarget) {
            char logSetting[32];
            sprintf(logSetting, "*:%s:%d", debugTarget, winvnc3.getInt("DebugLevel", 0));
            regKey.setString("Log", logSetting);
          }
 
          std::string authHosts = winvnc3.getString("AuthHosts", "");
          if (!authHosts.empty()) {
            std::string newHosts;

            // Reformat AuthHosts to Hosts.  Wish I'd left the format the same. :( :( :(
            try {
              // Split the AuthHosts string into patterns to match
              std::vector<std::string> patterns;
              patterns = rfb::split(authHosts.c_str(), ':');
              for (size_t i = 0; i < patterns.size(); i++) {
                if (!patterns[i].empty()) {
                  int bits = 0;
                  char pattern[1+4*4+4];
                  pattern[0] = patterns[i][0];
                  pattern[1] = 0;

                  // Split the pattern into IP address parts and process
                  std::vector<std::string> parts;
                  parts = rfb::split(&patterns[i][1], '.');
                  for (size_t j = 0; j < parts.size(); j++) {
                    if (bits)
                      strcat(pattern, ".");
                    if (parts[j].size() > 3)
                      throw rdr::Exception("Invalid IP address part");
                    if (!parts[j].empty()) {
                      strcat(pattern, parts[j].c_str());
                      bits += 8;
                    }
                  }

                  // Pad out the address specification if required
                  int addrBits = bits;
                  while (addrBits < 32) {
                    if (addrBits) strcat(pattern, ".");
                    strcat(pattern, "0");
                    addrBits += 8;
                  }

                  // Append the number of bits to match
                  char buf[4];
                  sprintf(buf, "/%d", bits);
                  strcat(pattern, buf);

                  // Append this pattern to the Hosts value
                  if (!newHosts.empty())
                    newHosts += ",";
                  newHosts += pattern;
                }
              }

              // Finally, save the Hosts value
              regKey.setString("Hosts", newHosts.c_str());
            } catch (rdr::Exception&) {
              MsgBox(nullptr, "Unable to convert AuthHosts setting to Hosts format.",
                     MB_ICONWARNING | MB_OK);
            }
          } else {
            regKey.setString("Hosts", "+");
          }

          regKey.setBool("LocalHost", winvnc3.getBool("LoopbackOnly", false));
          // *** check AllowLoopback?

          if (winvnc3.getBool("AuthRequired", true))
            regKey.setString("SecurityTypes", "VncAuth");
          else
            regKey.setString("SecurityTypes", "None");

          int connectPriority = winvnc3.getInt("ConnectPriority", 0);
          regKey.setBool("DisconnectClients", connectPriority == 0);
          regKey.setBool("AlwaysShared", connectPriority == 1);
          regKey.setBool("NeverShared", connectPriority == 2);

        } catch(rdr::Exception&) {
        }

        // Open the local, default-user settings
        allowProperties = true;
        try {
          RegKey userKey;
          userKey.openKey(winvnc3, "Default");
          vlog.info("loading Default prefs");
          LoadUserPrefs(userKey);
        } catch(rdr::Exception& e) {
          vlog.error("error reading Default settings:%s", e.str());
        }

        // Open the local, user-specific settings
        if (userSettings && !username.empty()) {
          try {
            RegKey userKey;
            userKey.openKey(winvnc3, username.c_str());
            vlog.info("loading local User prefs");
            LoadUserPrefs(userKey);
          } catch(rdr::Exception& e) {
            vlog.error("error reading local User settings:%s", e.str());
          }

          // Open the user's own settings
          if (allowProperties) {
            try {
              RegKey userKey;
              userKey.openKey(HKEY_CURRENT_USER, "Software\\ORL\\WinVNC3");
              vlog.info("loading global User prefs");
              LoadUserPrefs(userKey);
            } catch(rdr::Exception& e) {
              vlog.error("error reading global User settings:%s", e.str());
            }
          }
        }

        // Disable the Options menu item if appropriate
        regKey.setBool("DisableOptions", !allowProperties);
      }

      void LegacyPage::LoadUserPrefs(const RegKey& key)
      {
        regKey.setInt("PortNumber", key.getBool("SocketConnect") ? key.getInt("PortNumber", 5900) : 0);
        if (key.getBool("AutoPortSelect", false)) {
          MsgBox(nullptr, "The AutoPortSelect setting is not supported by this release."
                    "The port number will default to 5900.",
                    MB_ICONWARNING | MB_OK);
          regKey.setInt("PortNumber", 5900);
        }
        regKey.setInt("IdleTimeout", key.getInt("IdleTimeout", 0));

        regKey.setBool("RemoveWallpaper", key.getBool("RemoveWallpaper"));
        regKey.setBool("DisableEffects", key.getBool("DisableEffects"));

        if (key.getInt("QuerySetting", 2) != 2) {
          regKey.setBool("QueryConnect", key.getInt("QuerySetting") > 2);
          MsgBox(nullptr, "The QuerySetting option has been replaced by QueryConnect."
                 "Please see the documentation for details of the QueryConnect option.",
                 MB_ICONWARNING | MB_OK);
        }
        regKey.setInt("QueryTimeout", key.getInt("QueryTimeout", 10));

        std::vector<uint8_t> passwd;
        passwd = key.getBinary("Password");
        regKey.setBinary("Password", passwd.data(), passwd.size());

        bool enableInputs = key.getBool("InputsEnabled", true);
        regKey.setBool("AcceptKeyEvents", enableInputs);
        regKey.setBool("AcceptPointerEvents", enableInputs);
        regKey.setBool("AcceptCutText", enableInputs);
        regKey.setBool("SendCutText", enableInputs);

        switch (key.getInt("LockSetting", 0)) {
        case 0: regKey.setString("DisconnectAction", "None"); break;
        case 1: regKey.setString("DisconnectAction", "Lock"); break;
        case 2: regKey.setString("DisconnectAction", "Logoff"); break;
        };

        regKey.setBool("DisableLocalInputs", key.getBool("LocalInputsDisabled", false));

        // *** ignore polling preferences
        // PollUnderCursor, PollForeground, OnlyPollConsole, OnlyPollOnEvent
        regKey.setBool("UseHooks", !key.getBool("PollFullScreen", false));

        if (key.isValue("AllowShutdown"))
          MsgBox(nullptr, "The AllowShutdown option is not supported by this release.", MB_ICONWARNING | MB_OK);
        if (key.isValue("AllowEditClients"))
          MsgBox(nullptr, "The AllowEditClients option is not supported by this release.", MB_ICONWARNING | MB_OK);

        allowProperties = key.getBool("AllowProperties", allowProperties);
      }
