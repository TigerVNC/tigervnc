/* Copyright (C) 2016 Brian P. Hinz
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

import java.awt.image.*;
import java.io.*;
import java.nio.ByteBuffer;
import javax.imageio.*;
import javax.imageio.stream.*;

public class JpegDecompressor {

  public JpegDecompressor() {}

  public void decompress(ByteBuffer jpegBuf, int jpegBufLen,
    WritableRaster buf, Rect r, PixelFormat pf)
  {

    byte[] src = new byte[jpegBufLen];

    jpegBuf.get(src);
    try {
      BufferedImage image =
        ImageIO.read(new MemoryCacheImageInputStream(new ByteArrayInputStream(src)));
      ColorModel cm = pf.getColorModel();
      if (cm.isCompatibleRaster(image.getRaster()) &&
          cm.isCompatibleSampleModel(image.getRaster().getSampleModel())) {
        buf.setDataElements(0, 0, image.getRaster());
      } else {
        ColorConvertOp converter = pf.getColorConvertOp(cm.getColorSpace());
        converter.filter(image.getRaster(), buf);
      }
      image.flush();
    } catch (IOException e) {
      throw new Exception(e.getMessage());
    }
  }
}
