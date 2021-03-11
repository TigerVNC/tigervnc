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
// ClientParams - structure describing the current state of the remote client
//

#ifndef __RFB_CLIENTPARAMS_H__
#define __RFB_CLIENTPARAMS_H__

#include <set>

#include <rdr/types.h>
#include <rfb/Cursor.h>
#include <rfb/PixelFormat.h>
#include <rfb/ScreenSet.h>

namespace rfb {

  const int subsampleUndefined = -1;
  const int subsampleNone = 0;
  const int subsampleGray = 1;
  const int subsample2X = 2;
  const int subsample4X = 3;
  const int subsample8X = 4;
  const int subsample16X = 5;

  class ClientParams {
  public:
    ClientParams();
    ~ClientParams();

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

    const int width() const { return width_; }
    const int height() const { return height_; }
    const ScreenSet& screenLayout() const { return screenLayout_; }
    void setDimensions(int width, int height);
    void setDimensions(int width, int height, const ScreenSet& layout);

    const PixelFormat& pf() const { return pf_; }
    void setPF(const PixelFormat& pf);

    const char* name() const { return name_; }
    void setName(const char* name);

    const Cursor& cursor() const { return *cursor_; }
    void setCursor(const Cursor& cursor);

    const Point& cursorPos() const { return cursorPos_; }
    void setCursorPos(const Point& pos);

    bool supportsEncoding(rdr::S32 encoding) const;

    void setEncodings(int nEncodings, const rdr::S32* encodings);

    unsigned int ledState() { return ledState_; }
    void setLEDState(unsigned int state);

    rdr::U32 clipboardFlags() const { return clipFlags; }
    rdr::U32 clipboardSize(unsigned int format) const;
    void setClipboardCaps(rdr::U32 flags, const rdr::U32* lengths);

    // Wrappers to check for functionality rather than specific
    // encodings
    bool supportsLocalCursor() const;
    bool supportsCursorPosition() const;
    bool supportsDesktopSize() const;
    bool supportsLEDState() const;
    bool supportsFence() const;
    bool supportsContinuousUpdates() const;

    int compressLevel;
    int qualityLevel;
    int fineQualityLevel;
    int subsampling;

  private:

    int width_;
    int height_;
    ScreenSet screenLayout_;

    PixelFormat pf_;
    char* name_;
    Cursor* cursor_;
    Point cursorPos_;
    std::set<rdr::S32> encodings_;
    unsigned int ledState_;
    rdr::U32 clipFlags;
    rdr::U32 clipSizes[16];
  };
}
#endif
