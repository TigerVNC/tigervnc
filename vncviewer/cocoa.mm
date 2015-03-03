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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#define XK_LATIN1
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <rfb/keysymdef.h>
#include <rfb/XF86keysym.h>

#include "keysym2ucs.h"

#define NoSymbol 0

static bool captured = false;

int cocoa_capture_display(Fl_Window *win, bool all_displays)
{
  NSWindow *nsw;

  nsw = (NSWindow*)fl_xid(win);

  if (!captured) {
    if (all_displays) {
      if (CGCaptureAllDisplays() != kCGErrorSuccess)
        return 1;
    } else {
      CGDirectDisplayID displays[16];
      CGDisplayCount count;
      int index;

      if (CGGetActiveDisplayList(16, displays, &count) != kCGErrorSuccess)
        return 1;

      if (count != (unsigned)Fl::screen_count())
        return 1;

#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
      index = Fl::screen_num(win->x(), win->y(), win->w(), win->h());
#else
      index = 0;
#endif

      if (CGDisplayCapture(displays[index]) != kCGErrorSuccess)
        return 1;
    }

    captured = true;
  }

  if ([nsw level] == CGShieldingWindowLevel())
    return 0;

  [nsw setLevel:CGShieldingWindowLevel()];

  return 0;
}

void cocoa_release_display(Fl_Window *win)
{
  NSWindow *nsw;
  int newlevel;

  if (captured)
    CGReleaseAllDisplays();

  captured = false;

  nsw = (NSWindow*)fl_xid(win);

  // Someone else has already changed the level of this window
  if ([nsw level] != CGShieldingWindowLevel())
    return;

  // FIXME: Store the previous level somewhere so we don't have to hard
  //        code a level here.
#ifdef HAVE_FLTK_FULLSCREEN
  if (win->fullscreen_active() && win->contains(Fl::focus()))
    newlevel = NSStatusWindowLevel;
  else
#endif
    newlevel = NSNormalWindowLevel;

  // Only change if different as the level change also moves the window
  // to the top of that level.
  if ([nsw level] != newlevel)
    [nsw setLevel:newlevel];
}

int cocoa_is_keyboard_event(const void *event)
{
  NSEvent *nsevent;

  nsevent = (NSEvent*)event;

  switch ([nsevent type]) {
  case NSKeyDown:
  case NSKeyUp:
  case NSFlagsChanged:
    return 1;
  default:
    return 0;
  }
}

int cocoa_is_key_press(const void *event)
{
  NSEvent *nsevent;

  nsevent = (NSEvent*)event;

  if ([nsevent type] == NSKeyDown)
    return 1;

  if ([nsevent type] == NSFlagsChanged) {
    UInt32 mask;

    // We don't see any event on release of CapsLock
    if ([nsevent keyCode] == 0x39)
      return 1;

    // These are entirely undocumented, but I cannot find any other way
    // of differentiating between left and right keys
    switch ([nsevent keyCode]) {
    case 0x36:
      mask = 0x0010;
      break;
    case 0x37:
      mask = 0x0008;
      break;
    case 0x38:
      mask = 0x0002;
      break;
    case 0x39:
      // We don't see any event on release of CapsLock
      return 1;
    case 0x3A:
      mask = 0x0020;
      break;
    case 0x3B:
      mask = 0x0001;
      break;
    case 0x3C:
      mask = 0x0004;
      break;
    case 0x3D:
      mask = 0x0040;
      break;
    case 0x3E:
      mask = 0x2000;
      break;
    default:
      return 0;
    }

    if ([nsevent modifierFlags] & mask)
      return 1;
    else
      return 0;
  }

  return 0;
}

int cocoa_event_keycode(const void *event)
{
  NSEvent *nsevent;

  nsevent = (NSEvent*)event;

  return [nsevent keyCode];
}

static NSString *key_translate(UInt16 keyCode, UInt32 modifierFlags)
{
  const UCKeyboardLayout *layout;
  OSStatus err;

  layout = NULL;

#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5) || defined(__x86_64__)
  TISInputSourceRef keyboard;
  CFDataRef uchr;

  keyboard = TISCopyCurrentKeyboardInputSource();
  uchr = (CFDataRef)TISGetInputSourceProperty(keyboard,
                                              kTISPropertyUnicodeKeyLayoutData);
  if (uchr == NULL)
    return nil;

  layout = (const UCKeyboardLayout*)CFDataGetBytePtr(uchr);
#else // MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
  KeyboardLayoutRef old_layout;
  int kind;

  err = KLGetCurrentKeyboardLayout(&old_layout);
  if (err != noErr)
    return nil;

  err = KLGetKeyboardLayoutProperty(old_layout, kKLKind,
                                    (const void**)&kind);
  if (err != noErr)
    return nil;

  // Old, crufty layout format?
  if (kind == kKLKCHRKind) {
    void *kchr_layout;

    UInt32 chars, state;
    char buf[3];

    unichar result[16];
    ByteCount in_len, out_len;

    err = KLGetKeyboardLayoutProperty(old_layout, kKLKCHRData,
                                      (const void**)&kchr_layout);
    if (err != noErr)
      return nil;

    state = 0;

    keyCode &= 0x7f;
    modifierFlags &= 0xff00;

    chars = KeyTranslate(kchr_layout, keyCode | modifierFlags, &state);

    // Dead key?
    if (state != 0) {
      // We have no fool proof way of asking what dead key this is.
      // Assume we get a spacing equivalent if we press the
      // same key again, and try to deduce something from that.
      chars = KeyTranslate(kchr_layout, keyCode | modifierFlags, &state);
    }

    buf[0] = (chars >> 16) & 0xff;
    buf[1] = chars & 0xff;
    buf[2] = '\0';

    if (buf[0] == '\0') {
      buf[0] = buf[1];
      buf[1] = '\0';
    }

    // The data is now in some layout specific encoding. Need to convert
    // this to unicode.

    ScriptCode script;
    TextEncoding encoding;
    TECObjectRef converter;

    script = (ScriptCode)GetScriptManagerVariable(smKeyScript);

    err = UpgradeScriptInfoToTextEncoding(script, kTextLanguageDontCare,
                                          kTextRegionDontCare, NULL,
                                          &encoding);
    if (err != noErr)
      return nil;

    err = TECCreateConverter(&converter, encoding, kTextEncodingUnicodeV4_0);
    if (err != noErr)
      return nil;

    in_len = strlen(buf);
    out_len = sizeof(result);

    err = TECConvertText(converter, (ConstTextPtr)buf, in_len, &in_len,
                         (TextPtr)result, out_len, &out_len);

    TECDisposeConverter(converter);

    if (err != noErr)
      return nil;

    return [NSString stringWithCharacters:result
                     length:(out_len / sizeof(unichar))];
  }

  if ((kind != kKLKCHRuchrKind) && (kind != kKLuchrKind))
    return nil;

  err = KLGetKeyboardLayoutProperty(old_layout, kKLuchrData,
                                    (const void**)&layout);
  if (err != noErr)
    return nil;
#endif // MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5

  if (layout == NULL)
    return nil;

  UInt32 dead_state;
  UniCharCount max_len, actual_len;
  UniChar string[255];

  dead_state = 0;
  max_len = sizeof(string)/sizeof(*string);

  modifierFlags = (modifierFlags >> 8) & 0xff;

  err = UCKeyTranslate(layout, keyCode, kUCKeyActionDown, modifierFlags,
                       LMGetKbdType(), 0, &dead_state, max_len, &actual_len,
                       string);
  if (err != noErr)
    return nil;

  // Dead key?
  if (dead_state != 0) {
    // We have no fool proof way of asking what dead key this is.
    // Assume we get a spacing equivalent if we press the
    // same key again, and try to deduce something from that.
    err = UCKeyTranslate(layout, keyCode, kUCKeyActionDown, modifierFlags,
                         LMGetKbdType(), 0, &dead_state, max_len, &actual_len,
                         string);
    if (err != noErr)
      return nil;
  }

  return [NSString stringWithCharacters:string length:actual_len];
}

// FIXME: We use hard coded values here as the constants didn't appear
//        in the OS X headers until 10.5.
static const int kvk_map[][2] = {
  { 0x24, XK_Return },
  { 0x30, XK_Tab },
  { 0x31, XK_space },
  { 0x33, XK_BackSpace },
  { 0x35, XK_Escape },
  // This one is undocumented for unknown reasons
  { 0x36, XK_Super_R },
  { 0x37, XK_Super_L },
  { 0x38, XK_Shift_L },
  { 0x39, XK_Caps_Lock },
  { 0x3A, XK_Alt_L },
  { 0x3B, XK_Control_L },
  { 0x3C, XK_Shift_R },
  { 0x3D, XK_Alt_R },
  { 0x3E, XK_Control_R },
  { 0x40, XK_F17 },
  { 0x48, XF86XK_AudioRaiseVolume },
  { 0x49, XF86XK_AudioLowerVolume },
  { 0x4A, XF86XK_AudioMute },
  { 0x4F, XK_F18 },
  { 0x50, XK_F19 },
  { 0x5A, XK_F20 },
  { 0x60, XK_F5 },
  { 0x61, XK_F6 },
  { 0x62, XK_F7 },
  { 0x63, XK_F3 },
  { 0x64, XK_F8 },
  { 0x65, XK_F9 },
  { 0x67, XK_F11 },
  { 0x69, XK_F13 },
  { 0x6A, XK_F16 },
  { 0x6B, XK_F14 },
  { 0x6D, XK_F10 },
  // Also undocumented
  { 0x6E, XK_Menu },
  { 0x6F, XK_F12 },
  { 0x71, XK_F15 },
  // Should we send Insert here?
  { 0x72, XK_Help },
  { 0x73, XK_Home },
  { 0x74, XK_Page_Up },
  { 0x75, XK_Delete },
  { 0x76, XK_F4 },
  { 0x77, XK_End },
  { 0x78, XK_F2 },
  { 0x79, XK_Page_Down },
  { 0x7A, XK_F1 },
  { 0x7B, XK_Left },
  { 0x7C, XK_Right },
  { 0x7D, XK_Down },
  { 0x7E, XK_Up },
  // The OS X headers claim these keys are not layout independent.
  // Could it be because of the state of the decimal key?
  /* { 0x41, XK_KP_Decimal }, */ // see below
  { 0x43, XK_KP_Multiply },
  { 0x45, XK_KP_Add },
  // OS X doesn't have NumLock, so is this really correct?
  { 0x47, XK_Num_Lock },
  { 0x4B, XK_KP_Divide },
  { 0x4C, XK_KP_Enter },
  { 0x4E, XK_KP_Subtract },
  { 0x51, XK_KP_Equal },
  { 0x52, XK_KP_0 },
  { 0x53, XK_KP_1 },
  { 0x54, XK_KP_2 },
  { 0x55, XK_KP_3 },
  { 0x56, XK_KP_4 },
  { 0x57, XK_KP_5 },
  { 0x58, XK_KP_6 },
  { 0x59, XK_KP_7 },
  { 0x5B, XK_KP_8 },
  { 0x5C, XK_KP_9 },
};

int cocoa_event_keysym(const void *event)
{
  NSEvent *nsevent;

  UInt16 key_code;
  size_t i;

  NSString *chars;
  UInt32 modifiers;

  nsevent = (NSEvent*)event;

  key_code = [nsevent keyCode];

  // Start with keys that either don't generate a symbol, or
  // generate the same symbol as some other key.
  for (i = 0;i < sizeof(kvk_map)/sizeof(kvk_map[0]);i++) {
    if (key_code == kvk_map[i][0])
      return kvk_map[i][1];
  }

  // OS X always sends the same key code for the decimal key on the
  // num pad, but X11 wants different keysyms depending on if it should
  // be a comma or full stop.
  if (key_code == 0x41) {
    switch ([[nsevent charactersIgnoringModifiers] UTF8String][0]) {
    case ',':
      return XK_KP_Separator;
    case '.':
      return XK_KP_Decimal;
    default:
      return NoSymbol;
    }
  }

  // We want a "normal" symbol out of the event, which basically means
  // we only respect the shift and alt/altgr modifiers. Cocoa can help
  // us if we only wanted shift, but as we also want alt/altgr, we'll
  // have to do some lookup ourselves. This matches our behaviour on
  // other platforms.

  modifiers = 0;
  if ([nsevent modifierFlags] & NSAlphaShiftKeyMask)
    modifiers |= alphaLock;
  if ([nsevent modifierFlags] & NSShiftKeyMask)
    modifiers |= shiftKey;
  if ([nsevent modifierFlags] & NSAlternateKeyMask)
    modifiers |= optionKey;

  chars = key_translate(key_code, modifiers);
  if (chars == nil)
    return NoSymbol;

  // FIXME: Some dead keys are given as NBSP + combining character
  if ([chars length] != 1)
    return NoSymbol;

  // Dead key?
  if ([[nsevent characters] length] == 0)
    return ucs2keysym(ucs2combining([chars characterAtIndex:0]));

  return ucs2keysym([chars characterAtIndex:0]);
}
