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
    ListConnInfo() {
      Clear();
    };

    void Clear() {
      conn.clear();
      IP_address.clear();
      time_conn.clear();
      status.clear();
    };

    bool Empty() {
      return conn.empty();
    }

    void iBegin() {
      ci = conn.begin();
      Ii = IP_address.begin();
      ti = time_conn.begin();
      si = status.begin();
    }

    bool iEnd() {
      return ci == conn.end();
    }

    void iNext() {
      ci++;
      Ii++;
      ti++;
      si++;
    }

    void addInfo(DWORD Conn, char* IP, char* Time, int Status) {
      conn.push_front(Conn);
      IP_address.push_front(IP);
      time_conn.push_front(Time);
      status.push_front(Status);
    }

    void iGetCharInfo(char* buf[3]) {
      if (Empty())
        return;
      buf[0] = (*Ii);
      buf[1] = (*ti);
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
      };
    }

    DWORD iGetConn() { return *ci;};

    int iGetStatus() { return *si;};

    std::list<DWORD> conn;
    std::list<char*> IP_address;
    std::list<char*> time_conn;
    std::list<int> status;
    std::list<DWORD>::iterator ci;
    std::list<char*>::iterator Ii, ti;
    std::list<int>::iterator si;

  };

}
#endif

