/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2012 TigerVNC Team.
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

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;

public class ConnParams {
  static LogWriter vlog = new LogWriter("ConnParams");

  public ConnParams() {
    majorVersion = 0; minorVersion = 0;
    width = 0; height = 0; useCopyRect = false;
    supportsLocalCursor = false; supportsLocalXCursor = false;
    supportsDesktopResize = false; supportsExtendedDesktopSize = false;
    supportsDesktopRename = false; supportsLastRect = false;
    supportsSetDesktopSize = false; supportsFence = false;
    supportsContinuousUpdates = false;
    supportsClientRedirect = false;
    customCompressLevel = false; compressLevel = 6;
    noJpeg = false; qualityLevel = -1; fineQualityLevel = -1;
    subsampling = "SUBSAMP_UNDEFINED";
    name_ = null; nEncodings_ = 0; encodings_ = null;
    currentEncoding_ = Encodings.encodingRaw; verStrPos = 0;
    screenLayout = new ScreenSet();

    setName("");
  }

  public boolean readVersion(InStream is, Boolean done) 
  {
    if (verStrPos >= 12) return false;
    verStr = new StringBuilder(13);
    while (verStrPos < 12 && is.checkNoWait(1)) {
      verStr.insert(verStrPos++,(char)is.readU8());
    }

    if (verStrPos < 12) {
      done = Boolean.valueOf(false);
      return true;
    }
    done = Boolean.valueOf(true);
    verStr.insert(12,'0');
    verStrPos = 0;
    if (verStr.toString().matches("RFB \\d{3}\\.\\d{3}\\n0")) {
      majorVersion = Integer.parseInt(verStr.substring(4,7));
      minorVersion = Integer.parseInt(verStr.substring(8,11));
      return true;
    }
    return false;
  }

  public void writeVersion(OutStream os) {
    String str = String.format("RFB %03d.%03d\n", majorVersion, minorVersion);
    os.writeBytes(str.getBytes(), 0, 12);
    os.flush();
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

  public int width;
  public int height;
  public ScreenSet screenLayout;

  public PixelFormat pf() { return pf_; }
  public void setPF(PixelFormat pf) {
    pf_ = pf;
    if (pf.bpp != 8 && pf.bpp != 16 && pf.bpp != 32) {
      throw new Exception("setPF: not 8, 16 or 32 bpp?");
    }
  }

  public String name() { return name_; }
  public void setName(String name) 
  {
    name_ = name;
  }

  public int currentEncoding() { return currentEncoding_; }
  public int nEncodings() { return nEncodings_; }
  public int[] encodings() { return encodings_; }
  public void setEncodings(int nEncodings, int[] encodings)
  {
    if (nEncodings > nEncodings_) {
      encodings_ = new int[nEncodings];
    }
    nEncodings_ = nEncodings;
    useCopyRect = false;
    supportsLocalCursor = false;
    supportsDesktopResize = false;
    supportsExtendedDesktopSize = false;
    supportsLocalXCursor = false;
    supportsLastRect = false;
    customCompressLevel = false;
    compressLevel = -1;
    noJpeg = true;
    qualityLevel = -1;
    fineQualityLevel = -1;
    subsampling = "SUBSAMP_UNDEFINED";
    currentEncoding_ = Encodings.encodingRaw;

    for (int i = nEncodings-1; i >= 0; i--) {
      encodings_[i] = encodings[i];

      if (encodings[i] == Encodings.encodingCopyRect)
        useCopyRect = true;
      else if (encodings[i] == Encodings.pseudoEncodingCursor)
        supportsLocalCursor = true;
      else if (encodings[i] == Encodings.pseudoEncodingXCursor)
        supportsLocalXCursor = true;
      else if (encodings[i] == Encodings.pseudoEncodingDesktopSize)
        supportsDesktopResize = true;
      else if (encodings[i] == Encodings.pseudoEncodingExtendedDesktopSize)
        supportsExtendedDesktopSize = true;
      else if (encodings[i] == Encodings.pseudoEncodingDesktopName)
        supportsDesktopRename = true;
      else if (encodings[i] == Encodings.pseudoEncodingLastRect)
        supportsLastRect = true;
      else if (encodings[i] == Encodings.pseudoEncodingFence)
        supportsFence = true;
      else if (encodings[i] == Encodings.pseudoEncodingContinuousUpdates)
        supportsContinuousUpdates = true;
      else if (encodings[i] == Encodings.pseudoEncodingClientRedirect)
        supportsClientRedirect = true;
      else if (encodings[i] >= Encodings.pseudoEncodingCompressLevel0 &&
          encodings[i] <= Encodings.pseudoEncodingCompressLevel9) {
        customCompressLevel = true;
        compressLevel = encodings[i] - Encodings.pseudoEncodingCompressLevel0;
      } else if (encodings[i] >= Encodings.pseudoEncodingQualityLevel0 &&
          encodings[i] <= Encodings.pseudoEncodingQualityLevel9) {
        noJpeg = false;
        qualityLevel = encodings[i] - Encodings.pseudoEncodingQualityLevel0;
      } else if (encodings[i] <= Encodings.encodingMax &&
               Encoder.supported(encodings[i]))
        currentEncoding_ = encodings[i];
    }

    // If the TurboVNC fine quality/subsampling encodings exist, let them
    // override the coarse TightVNC quality level
    for (int i = nEncodings-1; i >= 0; i--) {
      if (encodings[i] >= Encodings.pseudoEncodingFineQualityLevel0 + 1 &&
          encodings[i] <= Encodings.pseudoEncodingFineQualityLevel100) {
        noJpeg = false;
        fineQualityLevel = encodings[i] - Encodings.pseudoEncodingFineQualityLevel0;
      } else if (encodings[i] >= Encodings.pseudoEncodingSubsamp1X &&
                 encodings[i] <= Encodings.pseudoEncodingSubsampGray) {
        noJpeg = false;
        subsampling = JpegCompressor.subsamplingName(encodings[i] - Encodings.pseudoEncodingSubsamp1X);
      }
    }
  }
  public boolean useCopyRect;

  public boolean supportsLocalCursor;
  public boolean supportsLocalXCursor;
  public boolean supportsDesktopResize;
  public boolean supportsExtendedDesktopSize;
  public boolean supportsDesktopRename;
  public boolean supportsClientRedirect;
  public boolean supportsFence;
  public boolean supportsContinuousUpdates;
  public boolean supportsLastRect;

  public boolean supportsSetDesktopSize;

  public boolean customCompressLevel;
  public int compressLevel;
  public boolean noJpeg;
  public int qualityLevel;
  public int fineQualityLevel;
  public String subsampling;

  private PixelFormat pf_;
  private String name_;
  private int nEncodings_;
  private int[] encodings_;
  private int currentEncoding_;
  private StringBuilder verStr;
  private int verStrPos;
}
