/* Copyright (C) 2026 TigerVNC Team
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
 */

package com.tigervnc.vncviewer;

import java.util.HashMap;
import java.util.Map;

import static java.awt.event.KeyEvent.*;

// Maps AWT VK_* virtual key codes to the numeric keycodes expected by the
// QEMU Extended Key Event RFB extension (pseudoEncodingQEMUKeyEvent). This
// is an approximation of the C++ viewer's platform-native scancodes
// (vncviewer/xkb_to_qnum.c, vncviewer/osx_to_qnum.c): Java's AWT/Swing
// event model has no public, cross-platform API for the real hardware
// scancode, so VK_* (already a reasonable cross-platform proxy for "which
// physical key", unlike a keysym) is used instead. The target numeric
// values below are taken directly from vncviewer/xkb_to_qnum.c's verified
// "to" values (its hex "to" field equals the decimal "qnum" in its own
// comments), not re-derived from memory. Coverage is the common
// 101/104-key layout; keys with no clean VK_* equivalent (JIS/international
// extras, multimedia keys, etc.) are not covered and fall back to 0,
// which callers should treat as "no keycode available".
public class QemuKeyMap {

  public static int toQnum(int vkCode, int keyLocation) {
    switch (vkCode) {
    case VK_SHIFT:
      return (keyLocation == KEY_LOCATION_RIGHT) ? 0x36 : 0x2a;
    case VK_CONTROL:
      return (keyLocation == KEY_LOCATION_RIGHT) ? 0x9d : 0x1d;
    case VK_ALT:
      return (keyLocation == KEY_LOCATION_RIGHT) ? 0xb8 : 0x38;
    case VK_ALT_GRAPH:
      return 0xb8;
    case VK_WINDOWS:
      return (keyLocation == KEY_LOCATION_RIGHT) ? 0xdc : 0xdb;
    case VK_ENTER:
      return (keyLocation == KEY_LOCATION_NUMPAD) ? 0x9c : 0x1c;
    }

    Integer qnum = codeMap.get(vkCode);
    return (qnum != null) ? qnum : 0;
  }

  private static final Map<Integer, Integer> codeMap = new HashMap<Integer, Integer>();
  static {
    // Letters
    codeMap.put(VK_A, 0x1e); codeMap.put(VK_S, 0x1f); codeMap.put(VK_D, 0x20);
    codeMap.put(VK_F, 0x21); codeMap.put(VK_G, 0x22); codeMap.put(VK_H, 0x23);
    codeMap.put(VK_J, 0x24); codeMap.put(VK_K, 0x25); codeMap.put(VK_L, 0x26);
    codeMap.put(VK_Q, 0x10); codeMap.put(VK_W, 0x11); codeMap.put(VK_E, 0x12);
    codeMap.put(VK_R, 0x13); codeMap.put(VK_T, 0x14); codeMap.put(VK_Y, 0x15);
    codeMap.put(VK_U, 0x16); codeMap.put(VK_I, 0x17); codeMap.put(VK_O, 0x18);
    codeMap.put(VK_P, 0x19);
    codeMap.put(VK_Z, 0x2c); codeMap.put(VK_X, 0x2d); codeMap.put(VK_C, 0x2e);
    codeMap.put(VK_V, 0x2f); codeMap.put(VK_B, 0x30); codeMap.put(VK_N, 0x31);
    codeMap.put(VK_M, 0x32);

    // Digit row
    codeMap.put(VK_1, 0x02); codeMap.put(VK_2, 0x03); codeMap.put(VK_3, 0x04);
    codeMap.put(VK_4, 0x05); codeMap.put(VK_5, 0x06); codeMap.put(VK_6, 0x07);
    codeMap.put(VK_7, 0x08); codeMap.put(VK_8, 0x09); codeMap.put(VK_9, 0x0a);
    codeMap.put(VK_0, 0x0b);
    codeMap.put(VK_MINUS, 0x0c); codeMap.put(VK_EQUALS, 0x0d);

    // Punctuation
    codeMap.put(VK_OPEN_BRACKET, 0x1a); codeMap.put(VK_CLOSE_BRACKET, 0x1b);
    codeMap.put(VK_SEMICOLON, 0x27); codeMap.put(VK_QUOTE, 0x28);
    codeMap.put(VK_BACK_SLASH, 0x2b);
    codeMap.put(VK_COMMA, 0x33); codeMap.put(VK_PERIOD, 0x34);
    codeMap.put(VK_SLASH, 0x35); codeMap.put(VK_BACK_QUOTE, 0x29);

    // Function keys
    codeMap.put(VK_F1, 0x3b); codeMap.put(VK_F2, 0x3c); codeMap.put(VK_F3, 0x3d);
    codeMap.put(VK_F4, 0x3e); codeMap.put(VK_F5, 0x3f); codeMap.put(VK_F6, 0x40);
    codeMap.put(VK_F7, 0x41); codeMap.put(VK_F8, 0x42); codeMap.put(VK_F9, 0x43);
    codeMap.put(VK_F10, 0x44); codeMap.put(VK_F11, 0x57); codeMap.put(VK_F12, 0x58);

    // Editing/whitespace
    codeMap.put(VK_TAB, 0x0f); codeMap.put(VK_BACK_SPACE, 0x0e);
    codeMap.put(VK_SPACE, 0x39); codeMap.put(VK_CAPS_LOCK, 0x3a);
    codeMap.put(VK_ESCAPE, 0x01); codeMap.put(VK_CONTEXT_MENU, 0xdd);

    // Navigation cluster
    codeMap.put(VK_UP, 0xc8); codeMap.put(VK_DOWN, 0xd0);
    codeMap.put(VK_LEFT, 0xcb); codeMap.put(VK_RIGHT, 0xcd);
    codeMap.put(VK_HOME, 0xc7); codeMap.put(VK_END, 0xcf);
    codeMap.put(VK_PAGE_UP, 0xc9); codeMap.put(VK_PAGE_DOWN, 0xd1);
    codeMap.put(VK_INSERT, 0xd2); codeMap.put(VK_DELETE, 0xd3);
    codeMap.put(VK_PRINTSCREEN, 0x54); codeMap.put(VK_SCROLL_LOCK, 0x46);
    codeMap.put(VK_PAUSE, 0xc6);

    // Numpad
    codeMap.put(VK_NUMPAD0, 0x52); codeMap.put(VK_NUMPAD1, 0x4f);
    codeMap.put(VK_NUMPAD2, 0x50); codeMap.put(VK_NUMPAD3, 0x51);
    codeMap.put(VK_NUMPAD4, 0x4b); codeMap.put(VK_NUMPAD5, 0x4c);
    codeMap.put(VK_NUMPAD6, 0x4d); codeMap.put(VK_NUMPAD7, 0x47);
    codeMap.put(VK_NUMPAD8, 0x48); codeMap.put(VK_NUMPAD9, 0x49);
    codeMap.put(VK_ADD, 0x4e); codeMap.put(VK_SUBTRACT, 0x4a);
    codeMap.put(VK_MULTIPLY, 0x37); codeMap.put(VK_DIVIDE, 0xb5);
    codeMap.put(VK_DECIMAL, 0x53); codeMap.put(VK_NUM_LOCK, 0x45);
  }
}
