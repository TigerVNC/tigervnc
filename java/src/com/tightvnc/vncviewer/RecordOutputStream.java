package com.tightvnc.vncviewer;

import java.io.DataOutput;
import java.io.IOException;

public class RecordOutputStream implements DataOutput {

  public RecordOutputStream(RecordInterface ri) {
    recordInterface = ri;
  }

  public void write(byte[] b) throws IOException {
    if (recordInterface.canWrite())
      recordInterface.write(b);
  }

  public void write(byte[] b, int off, int len) throws IOException {
    if (recordInterface.canWrite())
      recordInterface.write(b, off, len);
  }

  public void write(int b) throws IOException {
    if (recordInterface.canWrite())
      recordInterface.writeIntBE(b);
  }

  public void writeBoolean(boolean v) { }

  public void writeByte(int v) throws IOException {
    if (recordInterface.canWrite()) {
      recordInterface.writeByte((byte)v);
    }
  }

  public void writeBytes(String s) { }
  public void writeChar(int v) { }
  public void writeChars(String s) { }
  public void writeDouble(double v) { }
  public void writeFloat(float v) { }

  public void writeInt(int v) throws IOException {
    if (recordInterface.canWrite())
      recordInterface.writeIntBE(v);
  }

  public void writeLong(long v) { }
  public void writeShort(int v) { }
  public void writeUTF(String str) { }
  
  private RecordInterface recordInterface = null;
}
