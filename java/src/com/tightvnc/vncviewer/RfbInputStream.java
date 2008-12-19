package com.tightvnc.vncviewer;

import java.io.IOException;

public class RfbInputStream {
  RfbInputStream(RfbProto rfbProto) {
    rfb = rfbProto;
  }

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

  private RfbProto rfb = null;
}