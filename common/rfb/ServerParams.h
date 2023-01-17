/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2019 Pierre Ossman for Cendio AB
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
// ServerParams - structure describing the current state of the remote server
//

#ifndef __RFB_SERVERPARAMS_H__
#define __RFB_SERVERPARAMS_H__

#include <string>

#include <rfb/Cursor.h>
#include <rfb/PixelFormat.h>
#include <rfb/ScreenSet.h>

namespace rfb {

  class ServerParams {
  public:
    ServerParams();
    ~ServerParams();

    int majorVersion;
    int minorVersion;

    void setVersion(int major, int minor) {
      majorVersion = major; minorVersion = minor;
    }
    bool isVersion(int major, int minor) const {
      return majorVersion == major && minorVersion == minor;
    }
    bool beforeVersion(int major, int minor) const {
      return (majorVersion < major ||
              (majorVersion == major && minorVersion < minor));
    }
    bool afterVersion(int major, int minor) const {
      return !beforeVersion(major,minor+1);
    }

    int width() const { return width_; }
    int height() const { return height_; }
    const ScreenSet& screenLayout() const { return screenLayout_; }
    void setDimensions(int width, int height);
    void setDimensions(int width, int height, const ScreenSet& layout);

    const PixelFormat& pf() const { return pf_; }
    void setPF(const PixelFormat& pf);

    const char* name() const { return name_.c_str(); }
    void setName(const char* name);

    const Cursor& cursor() const { return *cursor_; }
    void setCursor(const Cursor& cursor);

    unsigned int ledState() { return ledState_; }
    void setLEDState(unsigned int state);

    uint32_t clipboardFlags() const { return clipFlags; }
    uint32_t clipboardSize(unsigned int format) const;
    void setClipboardCaps(uint32_t flags, const uint32_t* lengths);

    bool supportsQEMUKeyEvent;
    bool supportsSetDesktopSize;
    bool supportsFence;
    bool supportsContinuousUpdates;

  private:

    int width_;
    int height_;
    ScreenSet screenLayout_;

    PixelFormat pf_;
    std::string name_;
    Cursor* cursor_;
    unsigned int ledState_;
    uint32_t clipFlags;
    uint32_t clipSizes[16];
  };
}
#endif
