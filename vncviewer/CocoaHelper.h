// CocoaHelper.h — 原生窗口尺寸校正（ObjC++）
#ifndef VNCVIEWER_COCOAHELPER_H
#define VNCVIEWER_COCOAHELPER_H

class Fl_Window;

// 返回 1 表示窗口真实尺寸与期望不符（已强制校正）
int cocoa_reassert_window_size(Fl_Window *win, int w, int h);

#endif
