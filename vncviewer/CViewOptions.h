/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

// -=- CViewOptions.h

// Definition of the CViewOptions class, responsible for storing the
// current & requested VNCviewer options.

#ifndef __RFB_WIN32_CVIEW_OPTIONS_H__
#define __RFB_WIN32_CVIEW_OPTIONS_H__

#include <rfb/util.h>

namespace rfb {

  namespace win32 {

    //
    // -=- Options structure.  Each viewer option has a corresponding
    //     entry in CViewOptions.  The viewer options are set by calling
    //     CView::applyOptions(...)
    //     The CViewOptions structure automatically picks up the default
    //     value of each option from the Configuration system
    //     The readFromFile and writeFromFile methods can be used to load
    //     and save VNC configuration files.  readFromFile is backwards
    //     compatible with 3.3 releases, while writeToFile is not.

    class CViewOptions {
    public:
      CViewOptions();
      CViewOptions(const CViewOptions& o) {operator=(o);}
      CViewOptions& operator=(const CViewOptions& o);
      void readFromFile(const char* filename_);
      void writeToFile(const char* filename_);
      void writeDefaults();
      bool useLocalCursor;
      bool useDesktopResize;
      bool fullScreen;
      bool fullColour;
      int lowColourLevel;
      int preferredEncoding;
      bool autoSelect;
      bool shared;
      bool sendPtrEvents;
      bool sendKeyEvents;
      bool clientCutText;
      bool serverCutText;
      bool emulate3;
      int pointerEventInterval;
      bool protocol3_3;
      bool acceptBell;
      CharArray userName;
      void setUserName(const char* user);
      CharArray password;
      void setPassword(const char* pwd);
      CharArray configFileName;
      void setConfigFileName(const char* cfn);
      CharArray host;
      void setHost(const char* h);
      CharArray monitor;
      void setMonitor(const char* m);
      unsigned int menuKey;
      void setMenuKey(const char* keyName);
      char* menuKeyName();
      int qualityLevel;
      bool noJpeg;
    };


  };

};

#endif
