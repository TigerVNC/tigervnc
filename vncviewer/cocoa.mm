/* Copyright 2011-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <dlfcn.h>

#include <FL/Fl_Window.H>
#include <FL/x.H>

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

void cocoa_prevent_native_fullscreen(Fl_Window *win)
{
  NSWindow *nsw;
  nsw = (NSWindow*)fl_xid(win);
  assert(nsw);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 100700
  nsw.collectionBehavior |= NSWindowCollectionBehaviorFullScreenNone;
#endif
}

int cocoa_get_level(Fl_Window *win)
{
  NSWindow *nsw;
  nsw = (NSWindow*)fl_xid(win);
  assert(nsw);
  return [nsw level];
}

void cocoa_set_level(Fl_Window *win, int level)
{
  NSWindow *nsw;
  nsw = (NSWindow*)fl_xid(win);
  assert(nsw);
  [nsw setLevel:level];
}

static CFMachPortRef event_tap;
static CFRunLoopSourceRef tap_source;

static bool cocoa_is_trusted()
{
  CFStringRef keys[1];
  CFBooleanRef values[1];
  CFDictionaryRef options;

  Boolean trusted;

#if !defined(MAC_OS_X_VERSION_10_9) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_9
  // FIXME: Raise system requirements so this isn't needed
  void *lib;
  typedef Boolean (*AXIsProcessTrustedWithOptionsRef)(CFDictionaryRef);
  AXIsProcessTrustedWithOptionsRef AXIsProcessTrustedWithOptions;
  CFStringRef kAXTrustedCheckOptionPrompt;

  lib = dlopen(nullptr, 0);
  if (lib == nullptr)
    return false;

  AXIsProcessTrustedWithOptions = (AXIsProcessTrustedWithOptionsRef)dlsym(lib, "AXIsProcessTrustedWithOptions");

  dlclose(lib);

  if (AXIsProcessTrustedWithOptions == nullptr)
    return false;

  kAXTrustedCheckOptionPrompt = CFSTR("AXTrustedCheckOptionPrompt");
#endif

  keys[0] = kAXTrustedCheckOptionPrompt;
  values[0] = kCFBooleanTrue;
  options = CFDictionaryCreate(kCFAllocatorDefault,
                               (const void**)keys,
                               (const void**)values, 1,
                               &kCFCopyStringDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);
  if (options == nullptr)
    return false;

  trusted = AXIsProcessTrustedWithOptions(options);
  CFRelease(options);

  return trusted;
}

// http://atnan.com/blog/2012/02/29/modern-privileged-helper-tools-using-smjobbless-plus-xpc
// https://github.com/numist/Switch/issues/7
//

static CGEventRef cocoa_event_tap(CGEventTapProxy /*proxy*/,
                                  CGEventType type, CGEventRef event,
                                  void* /*refcon*/)
{
  fprintf(stderr, "Got event of type %d\n", type);

  if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp) && (type != kCGEventFlagsChanged))
    return event;

  fprintf(stderr, "Code: %d, Flags: 0x%08x\n",
          (int)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode),
          (int)CGEventGetFlags(event));

  fprintf(stderr, "Heading for PSN: %lld\n",
          CGEventGetIntegerValueField(event, kCGEventTargetProcessSerialNumber));

  fprintf(stderr, "Heading for PID: %lld (I am %d)\n",
          CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID),
          getpid());


#if 0
  ProcessSerialNumber psn;
  OSErr err;

  psn.highLongOfPSN = 0;
  psn.lowLongOfPSN = kCurrentProcess;

  fprintf(stderr, "0x%08x%08x\n",
          psn.highLongOfPSN, psn.lowLongOfPSN);

  err = GetCurrentProcess(&psn);
  fprintf(stderr, "GetCurrentProcess(0x%08x%08x) = %d\n",
          psn.highLongOfPSN, psn.lowLongOfPSN, err);

  CGEventPostToPSN(&psn, event);

  return nullptr;
#elif 1
  pid_t target;
  ProcessSerialNumber psn;
  OSErr err;

  target = CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID);
  if (target == getpid())
    return event;

  err = GetCurrentProcess(&psn);
  fprintf(stderr, "GetCurrentProcess(0x%08x%08x) = %d\n",
          psn.highLongOfPSN, psn.lowLongOfPSN, err);

  // FIXME: CGEventPostToPid() in macOS 10.11+
  CGEventPostToPSN(&psn, event);

  return nullptr;
#elif 1
  NSEvent* nsevent;

  nsevent = [NSEvent eventWithCGEvent:event];
  if (nsevent == nil)
    return event;

  [NSApp postEvent:nsevent atStart:NO];

  // Documentation is unclear on ownership here, but doing a release
  // here results in a crash, so I guess we shouldn't do that...
  //[nsevent release];

  fprintf(stderr, "Posted to NSApp\n");

  return nullptr;
#else
  return event;
#endif
}

bool cocoa_tap_keyboard()
{
  CGEventMask mask;

  if (event_tap != nullptr)
    return true;

  if (!cocoa_is_trusted())
    return false;

  mask = CGEventMaskBit(kCGEventKeyDown) |
         CGEventMaskBit(kCGEventKeyUp) |
         CGEventMaskBit(kCGEventFlagsChanged);

  // FIXME: Right modes/prio? (doesnt get Ctrl+arrows)
  // Test CGEventTapCreateForPSN()/ForPid()
  event_tap = CGEventTapCreate(kCGAnnotatedSessionEventTap,
                               kCGHeadInsertEventTap,
                               kCGEventTapOptionDefault,
                               mask, cocoa_event_tap, nullptr);
  if (event_tap == nullptr)
    return false;

  tap_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                             event_tap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), tap_source,
                     kCFRunLoopCommonModes);

  // FIXME: Needed?
  CGEventTapEnable(event_tap, true);

  return true;
}

// FIXME: Called in destructor?
void cocoa_untap_keyboard()
{
  if (event_tap == nullptr)
    return;

  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tap_source,
                        kCFRunLoopCommonModes);
  CFRelease(tap_source);
  tap_source = nullptr;

  CFRelease(event_tap);
  event_tap = nullptr;
}

CGColorSpaceRef cocoa_win_color_space(Fl_Window *win)
{
  NSWindow *nsw;
  NSColorSpace *nscs;

  nsw = (NSWindow*)fl_xid(win);
  assert(nsw);

  nscs = [nsw colorSpace];
  if (nscs == nil) {
    // Offscreen, so return standard SRGB color space
    assert(false);
    return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  }

  CGColorSpaceRef lut = [nscs CGColorSpace];

  // We want a permanent reference, not an autorelease
  CGColorSpaceRetain(lut);

  return lut;
}

bool cocoa_win_is_zoomed(Fl_Window *win)
{
  NSWindow *nsw;
  nsw = (NSWindow*)fl_xid(win);
  assert(nsw);
  return [nsw isZoomed];
}

void cocoa_win_zoom(Fl_Window *win)
{
  NSWindow *nsw;
  nsw = (NSWindow*)fl_xid(win);
  assert(nsw);
  [nsw zoom:nsw];
}
