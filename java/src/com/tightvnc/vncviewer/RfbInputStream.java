package com.tightvnc.vncviewer;

public class RfbInputStream {
  RfbInputStream(RfbProto rfbProto) {
    rfb = rfbProto;
  }

  private RfbProto rfb = null;
}