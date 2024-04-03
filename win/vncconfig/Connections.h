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
#ifndef WINVNCCONF_CONNECTIONS
#define WINVNCCONF_CONNECTIONS

#include <vector>

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/ModuleFileName.h>
#include <rfb/Configuration.h>
#include <rfb/Blacklist.h>
#include <rfb/util.h>
#include <network/TcpSocket.h>

static rfb::IntParameter port_number("PortNumber",
  "TCP/IP port on which the server will accept connections", 5900);
static rfb::StringParameter hosts("Hosts",
  "Filter describing which hosts are allowed access to this server", "+");
static rfb::BoolParameter localHost("LocalHost",
  "Only accept connections from via the local loop-back network interface", false);

namespace rfb {

  namespace win32 {

    class ConnHostDialog : public Dialog {
    public:
      ConnHostDialog() : Dialog(GetModuleHandle(nullptr)) {}
      bool showDialog(const char* pat) {
        pattern = pat;
        return Dialog::showDialog(MAKEINTRESOURCE(IDD_CONN_HOST));
      }
      void initDialog() override {
        if (pattern.empty())
          pattern = "+";

        if (pattern[0] == '+')
          setItemChecked(IDC_ALLOW, true);
        else if (pattern[0] == '?')
          setItemChecked(IDC_QUERY, true);
        else
          setItemChecked(IDC_DENY, true);

        setItemString(IDC_HOST_PATTERN, &pattern.c_str()[1]);
        pattern.clear();
      }
      bool onOk() override {
        std::string newPat;
        if (isItemChecked(IDC_ALLOW))
          newPat = '+';
        else if (isItemChecked(IDC_QUERY))
          newPat = '?';
        else
          newPat = '-';
        newPat += getItemString(IDC_HOST_PATTERN);

        try {
          network::TcpFilter::Pattern pat(network::TcpFilter::parsePattern(newPat.c_str()));
          pattern = network::TcpFilter::patternToStr(pat);
        } catch(rdr::Exception& e) {
          MsgBox(nullptr, e.str(), MB_ICONEXCLAMATION | MB_OK);
          return false;
        }
        return true;
      }
      const char* getPattern() {return pattern.c_str();}
    protected:
      std::string pattern;
    };

    class ConnectionsPage : public PropSheetPage {
    public:
      ConnectionsPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_CONNECTIONS)), regKey(rk) {}
      void initDialog() override {
        vlog.debug("set IDC_PORT %d", (int)port_number);
        setItemInt(IDC_PORT, port_number ? port_number : 5900);
        setItemChecked(IDC_RFB_ENABLE, port_number != 0);
        setItemInt(IDC_IDLE_TIMEOUT, rfb::Server::idleTimeout);
        setItemChecked(IDC_LOCALHOST, localHost);

        HWND listBox = GetDlgItem(handle, IDC_HOSTS);
        while (SendMessage(listBox, LB_GETCOUNT, 0, 0))
          SendMessage(listBox, LB_DELETESTRING, 0, 0);

        std::vector<std::string> hostv;
        hostv = split(hosts, ',');
        for (size_t i = 0; i < hostv.size(); i++) {
          if (!hostv[i].empty())
            SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)hostv[i].c_str());
        }

        onCommand(IDC_RFB_ENABLE, EN_CHANGE);
      }
      bool onCommand(int id, int cmd) override {
        switch (id) {
        case IDC_HOSTS:
          {
            DWORD selected = SendMessage(GetDlgItem(handle, IDC_HOSTS), LB_GETCURSEL, 0, 0);
            DWORD count = SendMessage(GetDlgItem(handle, IDC_HOSTS), LB_GETCOUNT, 0, 0);
            bool enable = selected != (DWORD)LB_ERR;
            enableItem(IDC_HOST_REMOVE, enable);
            enableItem(IDC_HOST_UP, enable && (selected > 0));
            enableItem(IDC_HOST_DOWN, enable && (selected+1 < count));
            enableItem(IDC_HOST_EDIT, enable);
            setChanged(isChanged());
          }
          return true;

        case IDC_PORT:
        case IDC_IDLE_TIMEOUT:
          if (cmd == EN_CHANGE)
            setChanged(isChanged());
          return false;

        case IDC_RFB_ENABLE:
        case IDC_LOCALHOST:
          {
            // RFB port
            enableItem(IDC_PORT, isItemChecked(IDC_RFB_ENABLE));

            // Hosts
            enableItem(IDC_LOCALHOST, isItemChecked(IDC_RFB_ENABLE));

            bool enableHosts = !isItemChecked(IDC_LOCALHOST) && isItemChecked(IDC_RFB_ENABLE);
            enableItem(IDC_HOSTS, enableHosts);
            enableItem(IDC_HOST_ADD, enableHosts);
            if (!enableHosts) {
              enableItem(IDC_HOST_REMOVE, enableHosts);
              enableItem(IDC_HOST_UP, enableHosts);
              enableItem(IDC_HOST_DOWN, enableHosts);
              enableItem(IDC_HOST_EDIT, enableHosts);
            } else {
              onCommand(IDC_HOSTS, EN_CHANGE);
            }
            setChanged(isChanged());
            return false;
          }

        case IDC_HOST_ADD:
          if (hostDialog.showDialog(""))
          {
            const char* pattern = hostDialog.getPattern();
            if (pattern)
              SendMessage(GetDlgItem(handle, IDC_HOSTS), LB_ADDSTRING, 0, (LPARAM)pattern);
          }
          return true;

        case IDC_HOST_EDIT:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            std::vector<char> pattern(SendMessage(listBox, LB_GETTEXTLEN, item, 0)+1);
            SendMessage(listBox, LB_GETTEXT, item, (LPARAM)pattern.data());

            if (hostDialog.showDialog(pattern.data())) {
              const char* newPat = hostDialog.getPattern();
              if (newPat) {
                item = SendMessage(listBox, LB_FINDSTRINGEXACT, item, (LPARAM)pattern.data());
                if (item != LB_ERR) {
                  SendMessage(listBox, LB_DELETESTRING, item, 0); 
                  SendMessage(listBox, LB_INSERTSTRING, item, (LPARAM)newPat);
                  SendMessage(listBox, LB_SETCURSEL, item, 0);
                  onCommand(IDC_HOSTS, EN_CHANGE);
                }
              }
            }
          }
          return true;

        case IDC_HOST_UP:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            std::vector<char> pattern(SendMessage(listBox, LB_GETTEXTLEN, item, 0)+1);
            SendMessage(listBox, LB_GETTEXT, item, (LPARAM)pattern.data());
            SendMessage(listBox, LB_DELETESTRING, item, 0);
            SendMessage(listBox, LB_INSERTSTRING, item-1, (LPARAM)pattern.data());
            SendMessage(listBox, LB_SETCURSEL, item-1, 0);
            onCommand(IDC_HOSTS, EN_CHANGE);
          }
          return true;

        case IDC_HOST_DOWN:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            std::vector<char> pattern(SendMessage(listBox, LB_GETTEXTLEN, item, 0)+1);
            SendMessage(listBox, LB_GETTEXT, item, (LPARAM)pattern.data());
            SendMessage(listBox, LB_DELETESTRING, item, 0);
            SendMessage(listBox, LB_INSERTSTRING, item+1, (LPARAM)pattern.data());
            SendMessage(listBox, LB_SETCURSEL, item+1, 0);
            onCommand(IDC_HOSTS, EN_CHANGE);
          }
          return true;

        case IDC_HOST_REMOVE:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            SendMessage(listBox, LB_DELETESTRING, item, 0);
            onCommand(IDC_HOSTS, EN_CHANGE);
          }

        }
        return false;
      }
      bool onOk() override {
        regKey.setInt("PortNumber", isItemChecked(IDC_RFB_ENABLE) ? getItemInt(IDC_PORT) : 0);
        regKey.setInt("IdleTimeout", getItemInt(IDC_IDLE_TIMEOUT));
        regKey.setInt("LocalHost", isItemChecked(IDC_LOCALHOST));
        regKey.setString("Hosts", getHosts().c_str());
        return true;
      }
      bool isChanged() {
        try {
          std::string new_hosts = getHosts();
          return (new_hosts != (const char*)hosts) ||
              (localHost != isItemChecked(IDC_LOCALHOST)) ||
              (port_number != getItemInt(IDC_PORT)) ||
              (rfb::Server::idleTimeout != getItemInt(IDC_IDLE_TIMEOUT));
        } catch (rdr::Exception&) {
          return false;
        }
      }
      std::string getHosts() {
        int bufLen = 1, i;
        HWND listBox = GetDlgItem(handle, IDC_HOSTS);
        for (i=0; i<SendMessage(listBox, LB_GETCOUNT, 0, 0); i++)
          bufLen+=SendMessage(listBox, LB_GETTEXTLEN, i, 0)+1;
        std::vector<char> hosts_str(bufLen);
        hosts_str[0] = 0;
        char* outPos = hosts_str.data();
        for (i=0; i<SendMessage(listBox, LB_GETCOUNT, 0, 0); i++) {
          outPos += SendMessage(listBox, LB_GETTEXT, i, (LPARAM)outPos);
          outPos[0] = ',';
          outPos[1] = 0;
          outPos++;
        }
        return hosts_str.data();
      }

    protected:
      RegKey regKey;
      ConnHostDialog hostDialog;
    };

  };

};

#endif
