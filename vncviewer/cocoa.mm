/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

static CFMachPortRef event_tap;
static CFRunLoopSourceRef tap_source;

// http://atnan.com/blog/2012/02/29/modern-privileged-helper-tools-using-smjobbless-plus-xpc
// https://github.com/numist/Switch/issues/7
// 

static CGEventRef cocoa_event_tap(CGEventTapProxy proxy,
                                  CGEventType type, CGEventRef event,
                                  void *refcon)
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

  psn.highLongOfPSN = 0;
  psn.lowLongOfPSN = kCurrentProcess;

  CGEventPostToPSN(&psn, event);
  return NULL;
#else
  return event;
#endif
}

int cocoa_tap_keyboard(Fl_Window *win)
{
  CFStringRef keys[1];
  CFBooleanRef values[1];
  CFDictionaryRef options;
  Boolean trusted;

  CGEventMask mask;

  if (event_tap != NULL)
    return 0;

//if (!AXIsProcessTrusted())
//  AXMakeProcessTrusted(

  // FIXME: macOS 10.9
  void *lib;
  typedef Boolean (*AXIsProcessTrustedWithOptionsRef)(CFDictionaryRef);
  AXIsProcessTrustedWithOptionsRef AXIsProcessTrustedWithOptions;
  CFStringRef kAXTrustedCheckOptionPrompt;

  lib = dlopen(NULL, 0);
  assert(lib);
  AXIsProcessTrustedWithOptions = (AXIsProcessTrustedWithOptionsRef)dlsym(lib, "AXIsProcessTrustedWithOptions");
  assert(AXIsProcessTrustedWithOptions);
  dlclose(lib);
  kAXTrustedCheckOptionPrompt = CFSTR("AXTrustedCheckOptionPrompt");


  keys[0] = kAXTrustedCheckOptionPrompt;
  values[0] = kCFBooleanTrue;
  options = CFDictionaryCreate(kCFAllocatorDefault,
                               (const void**)keys,
                               (const void**)values, 1,
                               &kCFCopyStringDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);
  if (options == NULL)
    return 1;
  CFShow(options);

  trusted = AXIsProcessTrustedWithOptions(options);
  CFRelease(options);

  if (!trusted) {
    NSWindow *nsw;
    nsw = (NSWindow*)fl_xid(win);
    [nsw setLevel:NSNormalWindowLevel];
    [nsw orderBack:nil];
    [NSApp deactivate];
    return 1;
  }

//NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
//BOOL accessibilityEnabled = AXIsProcessTrustedWithOptions((CFDictionaryRef)options);

  mask = CGEventMaskBit(kCGEventKeyDown) |
         CGEventMaskBit(kCGEventKeyUp) |
         CGEventMaskBit(kCGEventFlagsChanged);

  // FIXME: Right modes/prio?
  event_tap = CGEventTapCreate(kCGAnnotatedSessionEventTap,
                               kCGHeadInsertEventTap,
                               kCGEventTapOptionDefault,
                               mask, cocoa_event_tap, NULL);
  if (event_tap == NULL)
    return 1;

  tap_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                             event_tap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), tap_source,
                     kCFRunLoopCommonModes);

  // FIXME: Needed?
  CGEventTapEnable(event_tap, true);

  return 0;
}

// FIXME: Called in destructor?
void cocoa_untap_keyboard(Fl_Window *win)
{
  if (event_tap == NULL)
    return;

  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tap_source,
                        kCFRunLoopCommonModes);
  CFRelease(tap_source);
  tap_source = NULL;

  CFRelease(event_tap);
  event_tap = NULL;
}

CGColorSpaceRef cocoa_win_color_space(Fl_Window *win)
{
  NSWindow *nsw;
  NSColorSpace *nscs;

  nsw = (NSWindow*)fl_xid(win);

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
  return [nsw isZoomed];
}

void cocoa_win_zoom(Fl_Window *win)
{
  NSWindow *nsw;
  nsw = (NSWindow*)fl_xid(win);
  [nsw zoom:nsw];
}
