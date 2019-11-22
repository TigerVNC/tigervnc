/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2012-2019 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

public class ServerParams {

  private static final int subsampleUndefined = -1;
  private static final int subsampleNone = 0;
  private static final int subsampleGray = 1;
  private static final int subsample2X = 2;
  private static final int subsample4X = 3;
  private static final int subsample8X = 4;
  private static final int subsample16X = 5;

  public ServerParams()
  {
    majorVersion = 0; minorVersion = 0;
    width_ = 0; height_ = 0; useCopyRect = false;
    supportsLocalCursor = false; supportsLocalXCursor = false;
    supportsLocalCursorWithAlpha = false;
    supportsDesktopResize = false; supportsExtendedDesktopSize = false;
    supportsDesktopRename = false; supportsLastRect = false;
    supportsSetDesktopSize = false; supportsFence = false;
    supportsContinuousUpdates = false;
    supportsClientRedirect = false;
    compressLevel = 6; qualityLevel = -1; fineQualityLevel = -1;
    subsampling = subsampleUndefined; name_ = null; verStrPos = 0;

    encodings_ = new ArrayList();
    screenLayout_ = new ScreenSet();

    setName("");
  }

  public int majorVersion;
  public int minorVersion;

  public void setVersion(int major, int minor) {
    majorVersion = major; minorVersion = minor;
  }
  public boolean isVersion(int major, int minor) {
    return majorVersion == major && minorVersion == minor;
  }
  public boolean beforeVersion(int major, int minor) {
    return (majorVersion < major ||
            (majorVersion == major && minorVersion < minor));
  }
  public boolean afterVersion(int major, int minor) {
    return !beforeVersion(major,minor+1);
  }

  public int width() { return width_; }
  public int height() { return height_; }
  public ScreenSet screenLayout() { return screenLayout_; }

  public void setDimensions(int width, int height)
  {
    ScreenSet layout = new ScreenSet();
    layout.add_screen(new Screen(0, 0, 0, width, height, 0));
    setDimensions(width, height, layout);
  }

  public void setDimensions(int width, int height, ScreenSet layout)
  {
    if (!layout.validate(width, height))
      throw new Exception("Attempted to configure an invalid screen layout");

    width_ = width;
    height_ = height;
    screenLayout_ = layout;
  }

  public PixelFormat pf() { return pf_; }
  public void setPF(PixelFormat pf) {

    pf_ = pf;

    if (pf.bpp != 8 && pf.bpp != 16 && pf.bpp != 32)
      throw new Exception("setPF: not 8, 16 or 32 bpp?");
  }

  public String name() { return name_; }
  public void setName(String name)
  {
    name_ = name;
  }

  public boolean supportsEncoding(int encoding)
  {
    return encodings_.indexOf(encoding) != -1;
  }

  public void setEncodings(int nEncodings, int[] encodings)
  {
    useCopyRect = false;
    supportsLocalCursor = false;
    supportsLocalCursorWithAlpha = false;
    supportsDesktopResize = false;
    supportsExtendedDesktopSize = false;
    supportsLocalXCursor = false;
    supportsLastRect = false;
    compressLevel = -1;
    qualityLevel = -1;
    fineQualityLevel = -1;
    subsampling = subsampleUndefined;

    encodings_.clear();
    encodings_.add(Encodings.encodingRaw);

    for (int i = nEncodings-1; i >= 0; i--) {
      switch (encodings[i]) {
      case Encodings.encodingCopyRect:
        useCopyRect = true;
        break;
      case Encodings.pseudoEncodingCursor:
        supportsLocalCursor = true;
        break;
      case Encodings.pseudoEncodingXCursor:
        supportsLocalXCursor = true;
        break;
      case Encodings.pseudoEncodingCursorWithAlpha:
        supportsLocalCursorWithAlpha = true;
        break;
      case Encodings.pseudoEncodingDesktopSize:
        supportsDesktopResize = true;
        break;
      case Encodings.pseudoEncodingExtendedDesktopSize:
        supportsExtendedDesktopSize = true;
        break;
      case Encodings.pseudoEncodingDesktopName:
        supportsDesktopRename = true;
        break;
      case Encodings.pseudoEncodingLastRect:
        supportsLastRect = true;
        break;
      case Encodings.pseudoEncodingFence:
        supportsFence = true;
        break;
      case Encodings.pseudoEncodingContinuousUpdates:
        supportsContinuousUpdates = true;
        break;
      case Encodings.pseudoEncodingClientRedirect:
        supportsClientRedirect = true;
        break;
      case Encodings.pseudoEncodingSubsamp1X:
        subsampling = subsampleNone;
        break;
      case Encodings.pseudoEncodingSubsampGray:
        subsampling = subsampleGray;
        break;
      case Encodings.pseudoEncodingSubsamp2X:
        subsampling = subsample2X;
        break;
      case Encodings.pseudoEncodingSubsamp4X:
        subsampling = subsample4X;
        break;
      case Encodings.pseudoEncodingSubsamp8X:
        subsampling = subsample8X;
        break;
      case Encodings.pseudoEncodingSubsamp16X:
        subsampling = subsample16X;
        break;
      }

      if (encodings[i] >= Encodings.pseudoEncodingCompressLevel0 &&
          encodings[i] <= Encodings.pseudoEncodingCompressLevel9)
        compressLevel = encodings[i] - Encodings.pseudoEncodingCompressLevel0;

      if (encodings[i] >= Encodings.pseudoEncodingQualityLevel0 &&
          encodings[i] <= Encodings.pseudoEncodingQualityLevel9)
        qualityLevel = encodings[i] - Encodings.pseudoEncodingQualityLevel0;

      if (encodings[i] >= Encodings.pseudoEncodingFineQualityLevel0 &&
          encodings[i] <= Encodings.pseudoEncodingFineQualityLevel100)
        fineQualityLevel = encodings[i] - Encodings.pseudoEncodingFineQualityLevel0;

      if (encodings[i] > 0)
        encodings_.add(encodings[i]);
    }
  }

  public boolean useCopyRect;

  public boolean supportsLocalCursor;
  public boolean supportsLocalXCursor;
  public boolean supportsLocalCursorWithAlpha;
  public boolean supportsDesktopResize;
  public boolean supportsExtendedDesktopSize;
  public boolean supportsDesktopRename;
  public boolean supportsLastRect;
  public boolean supportsClientRedirect;

  public boolean supportsSetDesktopSize;
  public boolean supportsFence;
  public boolean supportsContinuousUpdates;

  public int compressLevel;
  public int qualityLevel;
  public int fineQualityLevel;
  public int subsampling;

  private int width_;
  private int height_;
  private ScreenSet screenLayout_;

  private PixelFormat pf_;
  private String name_;
  private Cursor cursor_;
  private ArrayList encodings_;
  private StringBuilder verStr;
  private int verStrPos;
}
