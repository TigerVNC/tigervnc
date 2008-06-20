//
//  Copyright (C) 2008 Wimba, Inc.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// FbsEntryPoint.java
//

package com.tightvnc.rfbplayer;

/**
 * Representation of an individual record of the .fbi (FrameBuffer Index) file.
 * It includes data that constitutes an entry point to indexed framebuffer
 * stream at a particular time offset.
 *
 * @author Constantin Kaplinsky
 */
public class FbsEntryPoint {

  /**
   * Timestamp in milliseconds corresponding to the keyframe data.
   * 32-bit unsigned integer.
   */
  public long timestamp;

  /**
   * Keyframe position in the respective .fbk file, offset in bytes from the
   * very beginning of the file. It should point to the byte counter of an FBS
   * data block. 32-bit unsigned integer.
   */
  public long key_fpos;

  /**
   * Keyframe data size in the respective .fbk file, in bytes. 32-bit unsigned
   * integer.
   */
  public long key_size;

  /**
   * Position of the next update in the .fbs file, offset in bytes from the
   * very beginning of the file. It should point to the byte counter of an FBS
   * data block. 32-bit unsigned integer.
   */
  public long fbs_fpos;

  /**
   * Offset in the FBS data block referenced by fbs_fpos. It allows addressing
   * those updates that begin not on the data block boundary. 32-bit unsigned
   * integer.
   */
  public long fbs_skip;

  /**
   * A replacement for {@link Object#toString()}.
   *
   * @return a string representation of the object.
   */
  public String toString() {
    String s = "[ timestamp:" + timestamp;
    s += " key_fpos:" + key_fpos;
    s += " key_size:" + key_size;
    s += " fbs_fpos:" + fbs_fpos;
    s += " fbs_skip:" + fbs_skip + " ]";

    return s;
  }

}
