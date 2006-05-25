/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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


#ifndef __RFB_LISTCONNINFO_INCLUDED__
#define __RFB_LISTCONNINFO_INCLUDED__

namespace rfb {

  struct ListConnInfo  {
    ListConnInfo() : disableClients(false) {}

    void Clear() {
      conn.clear();
      IP_address.clear();
      time_conn.clear();
      status.clear();
    }

    bool Empty() { return conn.empty();}

    void iBegin() {
      ci = conn.begin();
      Ii = IP_address.begin();
      ti = time_conn.begin();
      si = status.begin();
    }

    bool iEnd() { return ci == conn.end();}

    void iNext() {
      ci++;
      Ii++;
      ti++;
      si++;
    }

    void addInfo(void* Conn, char* IP, char* Time, int Status) {
      conn.push_back(Conn);
      IP_address.push_back(strDup(IP));
      time_conn.push_back(strDup(Time));
      status.push_back(Status);
    }

    void iGetCharInfo(char* buf[3]) {
      buf[0] = *Ii;
      buf[1] = *ti;
      switch (*si) {
      case 0:
        buf[2] = strDup("Full control");
        break;
      case 1:
        buf[2] = strDup("View only");
        break;
      case 2:
        buf[2] = strDup("Stop updating");
        break;
      default:
        buf[2] = strDup("Unknown");
      }
    }

    void* iGetConn() { return *ci;}

    int iGetStatus() { return *si;}

    void iSetStatus( int status) { *si = status;}

    void Copy(ListConnInfo* InputList) {
      Clear();
      if (InputList->Empty()) return;
      for (InputList->iBegin(); !InputList->iEnd(); InputList->iNext()) {
        iAdd(InputList);
      }
      setDisable(InputList->getDisable());
    }

    void iAdd (ListConnInfo* InputList) {
      char* buf[3];
      InputList->iGetCharInfo(buf);
      addInfo(InputList->iGetConn(), buf[0], buf[1], InputList->iGetStatus());
    }

    void setDisable(bool disable) {disableClients = disable;}

    bool getDisable() {return disableClients;}

    void setAllStatus(int stat) {
      std::list<int>::iterator st;
      for (st = status.begin(); st != status.end(); st++)
        *st = stat;
    }

  private:
    std::list<void*> conn;
    std::list<char*> IP_address;
    std::list<char*> time_conn;
    std::list<int> status;
    std::list<void*>::iterator ci;
    std::list<char*>::iterator Ii;
    std::list<char*>::iterator ti;
    std::list<int>::iterator si;
    bool disableClients;
  };
};
#endif

