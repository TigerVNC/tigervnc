package com.tightvnc.vncviewer;

import java.io.DataOutput;
import java.io.IOException;

public class RecordOutputStream implements DataOutput {

  public RecordOutputStream(RfbProto rfbproto) {
    rfb = rfbproto;
  }

  private boolean canWrite() {
    return ((rfb != null) && (rfb.rec != null));
  }

  public void write(byte[] b) throws IOException {
    if (canWrite())
      rfb.rec.write(b);
  }

  public void write(byte[] b, int off, int len) throws IOException {
    if (canWrite())
      rfb.rec.write(b, off, len);
  }

  public void write(int b) throws IOException {
    if (canWrite())
      rfb.rec.writeIntBE(b);
  }

  public void writeBoolean(boolean v) { }

  public void writeByte(int v) throws IOException {
    if (canWrite()) {
      rfb.rec.writeByte(v);
    }
  }

  public void writeBytes(String s) { }
  public void writeChar(int v) { }
  public void writeChars(String s) { }
  public void writeDouble(double v) { }
  public void writeFloat(float v) { }

  public void writeInt(int v) throws IOException {
    if (canWrite())
      rfb.rec.writeIntBE(v);
  }

  public void writeLong(long v) { }

  public void writeShort(int v) throws IOException {
    if (canWrite())
      rfb.rec.writeShortBE(v);
  }

  public void writeUTF(String str) { }

  private RfbProto rfb = null;
}
