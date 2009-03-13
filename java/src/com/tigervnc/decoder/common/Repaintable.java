package com.tigervnc.decoder.common;

public interface Repaintable {

  public void scheduleRepaint(int x, int y, int w, int h);

}
