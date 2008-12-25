package com.tightvnc.vncviewer;

import java.io.IOException;

//
// This class is layer between data of private RfbProto class
// and classes in other packages.
//
// For now this class is used by com.tightvnc.decoder.RawDecoder
//
public class RfbInputStream {
  RfbInputStream(RfbProto rfbProto) {
    rfb = rfbProto;
  }

  //
  // Read data methods
  //

  public void readFully(byte b[]) throws IOException {
    readFully(b, 0, b.length);
  }

  public void readFully(byte b[], int off, int len) throws IOException {
    rfb.readFully(b, off, len);
  }

  public int readU32() throws IOException  {
    return rfb.readU32();
  }

  public int readU8() throws IOException  {
    return rfb.readU8();
  }

  public int readCompactLen() throws IOException {
    return rfb.readCompactLen();
  }

  public int readU16() throws IOException {
    return rfb.readU16();
  }

  private RfbProto rfb = null;
}
