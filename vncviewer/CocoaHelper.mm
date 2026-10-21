// CocoaHelper.mm — 用原生 NSWindow setFrame 强制校正窗口尺寸
// 背景：FLTK 1.4 的 Cocoa 后端在窗口映射前后可能丢失 resize 请求，
// 导致 FLTK 认为自己是 WxH 而 WindowServer 里是 ~160x180 残窗。
// 看门狗周期性调用本函数，用 setFrame 硬校正并回报是否发生了校正。
#include <Cocoa/Cocoa.h>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <cstdlib>
#include "CocoaHelper.h"

int cocoa_reassert_window_size(Fl_Window *win, int w, int h)
{
  if (!win || !win->shown())
    return 0;

  NSWindow *nsw = (NSWindow *)fl_xid(win);
  if (!nsw)
    return 0;

  // 期望的内容尺寸（FLTK 坐标即点坐标）
  NSRect frame = [nsw frame];
  NSRect content = [nsw contentRectForFrameRect:frame];

  if ((abs((int)content.size.width - w) <= 2) &&
      (abs((int)content.size.height - h) <= 2))
    return 0;

  // 诊断+清除窗口尺寸约束（某些路径会把创建时的残窗尺寸设成
  // contentMaxSize，导致一切放大被 WindowServer 夹回）
  NSSize minSz = [nsw contentMinSize];
  NSSize maxSz = [nsw contentMaxSize];
  if (maxSz.width < w || maxSz.height < h) {
    fprintf(stderr, "DBG-cocoa: clearing contentMaxSize %.0fx%.0f\n",
            maxSz.width, maxSz.height);
    [nsw setContentMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
  }
  if (minSz.width > w || minSz.height > h) {
    fprintf(stderr, "DBG-cocoa: clearing contentMinSize %.0fx%.0f\n",
            minSz.width, minSz.height);
    [nsw setContentMinSize:NSMakeSize(1, 1)];
  }

  // 保持左下角位置不变，设置新内容尺寸
  NSRect newContent = content;
  newContent.size.width = w;
  newContent.size.height = h;
  NSRect newFrame = [nsw frameRectForContentRect:newContent];
  [nsw setFrame:newFrame display:YES animate:NO];

  // macOS 27 / FLTK 分裂修复：NSWindow 框架可能已变大，但
  // contentView 视图仍停留在初始残窗尺寸，WindowServer 按视图
  // 尺寸合成 → 屏幕上永远是残窗。把视图也一并掰到位。
  NSView *cv = [nsw contentView];
  if (cv) {
    NSRect viewFrame = [cv frame];
    if ((abs((int)viewFrame.size.width - w) > 2) ||
        (abs((int)viewFrame.size.height - h) > 2)) {
      fprintf(stderr, "DBG-cocoa: fixing contentView %.0fx%.0f -> %dx%d\n",
              viewFrame.size.width, viewFrame.size.height, w, h);
      [cv setAutoresizesSubviews:YES];
      [cv setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
      [cv setFrame:NSMakeRect(0, 0, w, h)];
      [cv setNeedsDisplay:YES];
    }
  }

  fprintf(stderr, "DBG-cocoa: winNum=%ld visible=%d activeSpace=%d setFrame content %dx%d (was %.0fx%.0f min %.0fx%.0f max %.0fx%.0f)\n",
          (long)[nsw windowNumber], [nsw isVisible], [nsw isOnActiveSpace],
          w, h, content.size.width, content.size.height,
          minSz.width, minSz.height, maxSz.width, maxSz.height);

  return 1;
}
