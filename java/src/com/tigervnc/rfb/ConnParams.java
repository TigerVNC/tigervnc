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
    supportsSetDesktopSize = false;
    customCompressLevel = false; compressLevel = 6;
    noJpeg = false; qualityLevel = -1; 
    name_ = null; nEncodings_ = 0; encodings_ = null;
    currentEncoding_ = Encodings.encodingRaw; verStrPos = 0;
    screenLayout = new ScreenSet();

    setName("");
  }

  public boolean readVersion(InStream is, boolean done) {
    if (verStrPos >= 12) return false;
    byte[] verStr = new byte[13];
    while (verStrPos < 12) {
      verStr[verStrPos++] = (byte)is.readU8();
    }

    if (verStrPos < 12) {
      done = false;
      return true;
    }
    done = true;
    verStr[12] = 0;
    majorVersion = (verStr[4] - '0') * 100 + (verStr[5] - '0') * 10 + (verStr[6] - '0');
    minorVersion = (verStr[8] - '0') * 100 + (verStr[9] - '0') * 10 + (verStr[10] - '0');
    verStrPos = 0;
    return true;
  }

  public void writeVersion(OutStream os) {
    byte[] b = new byte[12];
    b[0] = (byte)'R'; b[1] = (byte)'F'; b[2] = (byte)'B'; b[3] = (byte)' ';
    b[4] = (byte)('0' + (majorVersion / 100) % 10);
    b[5] = (byte)('0' + (majorVersion / 10) % 10);
    b[6] = (byte)('0' + majorVersion % 10);
    b[7] = (byte)'.';
    b[8] = (byte)('0' + (minorVersion / 100) % 10);
    b[9] = (byte)('0' + (minorVersion / 10) % 10);
    b[10] = (byte)('0' + minorVersion % 10);
    b[11] = (byte)'\n';
    os.writeBytes(b, 0, 12);
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
    customCompressLevel = false;
    compressLevel = -1;
    noJpeg = true;
    qualityLevel = -1;
    currentEncoding_ = Encodings.encodingRaw;

    for (int i = nEncodings-1; i >= 0; i--) {
      encodings_[i] = encodings[i];
      if (encodings[i] == Encodings.encodingCopyRect)
        useCopyRect = true;
      else if (encodings[i] == Encodings.pseudoEncodingCursor)
        supportsLocalCursor = true;
      else if (encodings[i] == Encodings.pseudoEncodingDesktopSize)
        supportsDesktopResize = true;
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
  }
  public boolean useCopyRect;

  public boolean supportsLocalCursor;
  public boolean supportsLocalXCursor;
  public boolean supportsDesktopResize;
  public boolean supportsExtendedDesktopSize;
  public boolean supportsDesktopRename;
  public boolean supportsLastRect;

  public boolean supportsSetDesktopSize;

  public boolean customCompressLevel;
  public int compressLevel;
  public boolean noJpeg;
  public int qualityLevel;

  private PixelFormat pf_;
  private String name_;
  private int nEncodings_;
  private int[] encodings_;
  private int currentEncoding_;
  private String verStr;
  private int verStrPos;
}
