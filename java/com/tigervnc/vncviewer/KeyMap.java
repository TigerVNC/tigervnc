/* Copyright (C) 2017 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;

import com.tigervnc.rfb.*;

import static java.awt.event.KeyEvent.*;
import static com.tigervnc.rfb.Keysymdef.*;

public class KeyMap {

  public final static int NoSymbol = 0;

  private static final HashMap<Integer, Character> code_map_java_to_char;
  static {
    // Certain KeyStrokes fail to produce a valid character (CTRL+ALT+...
    // on Windows and Mac almost never does).  For these cases, the best
    // we can try to do is to map to the ASCII symbol from the keyCode.
    code_map_java_to_char = new HashMap<Integer, Character>();
    for (int c=32; c<0x7f; c++) {
      int keyCode = KeyEvent.getExtendedKeyCodeForChar(c);
      if (keyCode != KeyEvent.VK_UNDEFINED)
        // Not all ASCII characters have VK_ constants, see vk_to_ascii()
        code_map_java_to_char.put(keyCode, (char)c);
    }
  }

  private static int[][] vkey_map = {
    /* KEYCODE                                                LOCATION                                      */
    /*                          UNKNOWN       STANDARD                  LEFT          RIGHT         NUMPAD  */
    { VK_BACK_SPACE,            NoSymbol,     XK_BackSpace,             NoSymbol,     NoSymbol,     NoSymbol },
    { VK_TAB,                   NoSymbol,     XK_Tab,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_CANCEL,                NoSymbol,     XK_Cancel,                NoSymbol,     NoSymbol,     NoSymbol },
    { VK_ENTER,                 NoSymbol,     XK_Return,                NoSymbol,     NoSymbol,     XK_KP_Enter },
    { VK_SHIFT,                 NoSymbol,     XK_Shift_L,               XK_Shift_L,   XK_Shift_R,   NoSymbol },
    { VK_CONTROL,               NoSymbol,     XK_Control_L,             XK_Control_L, XK_Control_R, NoSymbol },
    { VK_ALT,                   NoSymbol,     XK_Alt_L,                 XK_Alt_L,     XK_Alt_R,     NoSymbol },
    /* VK_PAUSE left out on purpose because interpretation depends on state of CTRL. See further down. */
    { VK_CAPS_LOCK,             NoSymbol,     XK_Caps_Lock,             NoSymbol,     NoSymbol,     NoSymbol },
    { VK_ESCAPE,                NoSymbol,     XK_Escape,                NoSymbol,     NoSymbol,     NoSymbol },
    { VK_END,                   NoSymbol,     XK_End,                   NoSymbol,     NoSymbol,     XK_KP_End },
    { VK_HOME,                  NoSymbol,     XK_Home,                  NoSymbol,     NoSymbol,     XK_KP_Home },
    { VK_LEFT,                  NoSymbol,     XK_Left,                  NoSymbol,     NoSymbol,     XK_KP_Left },
    { VK_UP,                    NoSymbol,     XK_Up,                    NoSymbol,     NoSymbol,     XK_KP_Up },
    { VK_RIGHT,                 NoSymbol,     XK_Right,                 NoSymbol,     NoSymbol,     XK_KP_Right },
    { VK_DOWN,                  NoSymbol,     XK_Down,                  NoSymbol,     NoSymbol,     XK_KP_Down },
    /* VK_PRINTSCREEN left out on purpose because interpretation depends on state of CTRL. See further down. */
    { VK_PAGE_UP,               NoSymbol,     XK_Page_Up,               NoSymbol,     NoSymbol,     XK_KP_Page_Up },
    { VK_PAGE_DOWN,             NoSymbol,     XK_Page_Down,             NoSymbol,     NoSymbol,     XK_KP_Page_Down },
    { VK_BEGIN,                 NoSymbol,     XK_Begin,                 NoSymbol,     NoSymbol,     XK_KP_Begin },
    { VK_KP_LEFT,               NoSymbol,     XK_KP_Left,               NoSymbol,     NoSymbol,     XK_KP_Left },
    { VK_KP_UP,                 NoSymbol,     XK_KP_Up,                 NoSymbol,     NoSymbol,     XK_KP_Up },
    { VK_KP_RIGHT,              NoSymbol,     XK_KP_Right,              NoSymbol,     NoSymbol,     XK_KP_Right },
    { VK_KP_DOWN,               NoSymbol,     XK_KP_Down,               NoSymbol,     NoSymbol,     XK_KP_Down },
    { VK_INSERT,                NoSymbol,     XK_Insert,                NoSymbol,     NoSymbol,     XK_KP_Insert },
    { VK_DELETE,                NoSymbol,     XK_Delete,                NoSymbol,     NoSymbol,     XK_KP_Delete },
    { VK_WINDOWS,               NoSymbol,     NoSymbol,                 XK_Super_L,   XK_Super_R,   NoSymbol },
    { VK_CONTEXT_MENU,          NoSymbol,     XK_Menu,                  NoSymbol,     NoSymbol,     NoSymbol },
    { VK_NUMPAD0,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_0 },
    { VK_NUMPAD1,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_1 },
    { VK_NUMPAD2,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_2 },
    { VK_NUMPAD3,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_3 },
    { VK_NUMPAD4,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_4 },
    { VK_NUMPAD5,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_5 },
    { VK_NUMPAD6,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_6 },
    { VK_NUMPAD7,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_7 },
    { VK_NUMPAD8,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_8 },
    { VK_NUMPAD9,               NoSymbol,     NoSymbol,                 NoSymbol,     NoSymbol,     XK_KP_9 },
    { VK_MULTIPLY,              NoSymbol,     XK_KP_Multiply,           NoSymbol,     NoSymbol,     XK_KP_Multiply },
    { VK_ADD,                   NoSymbol,     XK_KP_Add,                NoSymbol,     NoSymbol,     XK_KP_Add },
    { VK_SUBTRACT,              NoSymbol,     XK_KP_Subtract,           NoSymbol,     NoSymbol,     XK_KP_Subtract },
    { VK_DIVIDE,                NoSymbol,     XK_KP_Divide,             NoSymbol,     NoSymbol,     XK_KP_Divide },
    { VK_SEPARATER,             NoSymbol,     XK_KP_Separator,          NoSymbol,     NoSymbol,     XK_KP_Separator },
    { VK_DECIMAL,               NoSymbol,     XK_KP_Decimal,            NoSymbol,     NoSymbol,     XK_KP_Decimal },
    { VK_F1,                    NoSymbol,     XK_F1,                    XK_L1,        XK_R1,        NoSymbol },
    { VK_F2,                    NoSymbol,     XK_F2,                    XK_L2,        XK_R2,        NoSymbol },
    { VK_F3,                    NoSymbol,     XK_F3,                    XK_L3,        XK_R3,        NoSymbol },
    { VK_F4,                    NoSymbol,     XK_F4,                    XK_L4,        XK_R4,        NoSymbol },
    { VK_F5,                    NoSymbol,     XK_F5,                    XK_L5,        XK_R5,        NoSymbol },
    { VK_F6,                    NoSymbol,     XK_F6,                    XK_L6,        XK_R6,        NoSymbol },
    { VK_F7,                    NoSymbol,     XK_F7,                    XK_L7,        XK_R7,        NoSymbol },
    { VK_F8,                    NoSymbol,     XK_F8,                    XK_L8,        XK_R8,        NoSymbol },
    { VK_F9,                    NoSymbol,     XK_F9,                    XK_L9,        XK_R9,        NoSymbol },
    { VK_F10,                   NoSymbol,     XK_F10,                   XK_L10,       XK_R10,       NoSymbol },
    { VK_F11,                   NoSymbol,     XK_F11,                   NoSymbol,     XK_R11,       NoSymbol },
    { VK_F12,                   NoSymbol,     XK_F12,                   NoSymbol,     XK_R12,       NoSymbol },
    { VK_F13,                   NoSymbol,     XK_F13,                   NoSymbol,     XK_R13,       NoSymbol },
    { VK_F14,                   NoSymbol,     XK_F14,                   NoSymbol,     XK_R14,       NoSymbol },
    { VK_F15,                   NoSymbol,     XK_F15,                   NoSymbol,     XK_R15,       NoSymbol },
    { VK_F16,                   NoSymbol,     XK_F16,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F17,                   NoSymbol,     XK_F17,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F18,                   NoSymbol,     XK_F18,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F19,                   NoSymbol,     XK_F19,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F20,                   NoSymbol,     XK_F20,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F21,                   NoSymbol,     XK_F21,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F22,                   NoSymbol,     XK_F22,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F23,                   NoSymbol,     XK_F23,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_F24,                   NoSymbol,     XK_F24,                   NoSymbol,     NoSymbol,     NoSymbol },
    { VK_NUM_LOCK,              NoSymbol,     XK_Num_Lock,              NoSymbol,     NoSymbol,     XK_Num_Lock },
    { VK_SCROLL_LOCK,           NoSymbol,     XK_Scroll_Lock,           NoSymbol,     NoSymbol,     NoSymbol },
    { VK_ALT_GRAPH,             NoSymbol,     XK_ISO_Level3_Shift,      NoSymbol,     NoSymbol,     NoSymbol },
    { VK_META,                  NoSymbol,     NoSymbol,                 XK_Meta_L,    XK_Meta_R,    NoSymbol },
    { VK_MODECHANGE,            NoSymbol,     XK_Mode_switch,           NoSymbol,     NoSymbol,     NoSymbol },
    { VK_CLEAR,                 NoSymbol,     XK_Clear,                 NoSymbol,     NoSymbol,     XK_KP_Begin },
    { VK_AGAIN,                 NoSymbol,     XK_Redo,                  NoSymbol,     NoSymbol,     NoSymbol },
    { VK_UNDO,                  NoSymbol,     XK_Undo,                  NoSymbol,     NoSymbol,     NoSymbol },
    { VK_FIND,                  NoSymbol,     XK_Find,                  NoSymbol,     NoSymbol,     NoSymbol },
    { VK_STOP,                  NoSymbol,     XK_Cancel,                NoSymbol,     NoSymbol,     NoSymbol },
    { VK_HELP,                  NoSymbol,     XK_Help,                  NoSymbol,     NoSymbol,     NoSymbol },
    { VK_KANJI,                 NoSymbol,     XK_Kanji,                 NoSymbol,     NoSymbol,     NoSymbol },
    { VK_KATAKANA,              NoSymbol,     XK_Katakana,              NoSymbol,     NoSymbol,     NoSymbol },
    { VK_HIRAGANA,              NoSymbol,     XK_Hiragana,              NoSymbol,     NoSymbol,     NoSymbol },
    { VK_PREVIOUS_CANDIDATE,    NoSymbol,     XK_PreviousCandidate,     NoSymbol,     NoSymbol,     NoSymbol },
    { VK_CODE_INPUT,            NoSymbol,     XK_Codeinput,             NoSymbol,     NoSymbol,     NoSymbol },
    { VK_JAPANESE_ROMAN,        NoSymbol,     XK_Romaji,                NoSymbol,     NoSymbol,     NoSymbol },
    { VK_KANA_LOCK,             NoSymbol,     XK_Kana_Lock,             NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_ABOVEDOT,         NoSymbol,     XK_dead_abovedot,         NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_ABOVERING,        NoSymbol,     XK_dead_abovering,        NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_ACUTE,            NoSymbol,     XK_dead_acute,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_BREVE,            NoSymbol,     XK_dead_breve,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_CARON,            NoSymbol,     XK_dead_caron,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_CEDILLA,          NoSymbol,     XK_dead_cedilla,          NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_CIRCUMFLEX,       NoSymbol,     XK_dead_circumflex,       NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_DIAERESIS,        NoSymbol,     XK_dead_diaeresis,        NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_DOUBLEACUTE,      NoSymbol,     XK_dead_doubleacute,      NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_GRAVE,            NoSymbol,     XK_dead_grave,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_IOTA,             NoSymbol,     XK_dead_iota,             NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_MACRON,           NoSymbol,     XK_dead_macron,           NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_OGONEK,           NoSymbol,     XK_dead_ogonek,           NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_SEMIVOICED_SOUND, NoSymbol,     XK_dead_semivoiced_sound, NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_TILDE,            NoSymbol,     XK_dead_tilde,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_DEAD_VOICED_SOUND,     NoSymbol,     XK_dead_voiced_sound,     NoSymbol,     NoSymbol,     NoSymbol },
    { VK_ALPHANUMERIC,          NoSymbol,     XK_Eisu_Shift,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_ALL_CANDIDATES,        NoSymbol,     XK_MultipleCandidate,     NoSymbol,     NoSymbol,     NoSymbol },
    { VK_KANA,                  NoSymbol,     XK_Kana_Shift,            NoSymbol,     NoSymbol,     NoSymbol },
    { VK_JAPANESE_KATAKANA,     NoSymbol,     XK_Katakana,              NoSymbol,     NoSymbol,     NoSymbol },
    { VK_JAPANESE_HIRAGANA,     NoSymbol,     XK_Hiragana,              NoSymbol,     NoSymbol,     NoSymbol },
    { VK_COMPOSE,               NoSymbol,     XK_Multi_key,             NoSymbol,     NoSymbol,     NoSymbol },
  };

  public static int vkey_to_keysym(KeyEvent ev)
  {
    int keyCode = get_keycode_fallback_extended(ev);

    // Start with keys that either don't generate a symbol, or
    // generate the same symbol as some other key.
    if (keyCode == KeyEvent.VK_PAUSE)
      return (ev.isControlDown() ? XK_Break : XK_Pause);
    else if (keyCode == KeyEvent.VK_PRINTSCREEN)
      return (ev.isControlDown() ? XK_Sys_Req : XK_Print);
    else
      for(int i = 0; i < vkey_map.length; i++)
        if (keyCode == vkey_map[i][0])
          return vkey_map[i][ev.getKeyLocation()+1];

    // Unknown special key?
    if (KeyEvent.getKeyText(keyCode).isEmpty()) {
      vlog.error("Unknown key code: 0x%04x", keyCode);
      return NoSymbol;
    }

    // Pressing Ctrl wreaks havoc with the symbol lookup...
    int ucs = (int)ev.getKeyChar();
    if (ev.isControlDown()) {
      // For CTRL-<letter>, CTRL is sent separately, so just send <letter>.
      if ((ucs >= 1 && ucs <= 26 && !ev.isShiftDown()) ||
          // CTRL-{, CTRL-|, CTRL-} also map to ASCII 96-127
          (ucs >= 27 && ucs <= 29 && ev.isShiftDown()))
        ucs += 96;
      // For CTRL-SHIFT-<letter>, send capital <letter> to emulate behavior
      // of Linux.  For CTRL-@, send @.  For CTRL-_, send _.  For CTRL-^,
      // send ^.
      else if (ucs < 32)
        ucs += 64;
      // If it's still undefined, map the keyCode to ASCII symbol
      else if (keyCode >= 0 && keyCode <= 127)
        if (ucs == CHAR_UNDEFINED || ev.isAltDown())
          ucs = vk_to_ascii(keyCode, ev.isShiftDown());
        else if (VncViewer.os.startsWith("mac os x") && ev.isMetaDown()) 
        // Alt on OS X behaves more like AltGr on other systems, and to get
        // sane behaviour we should translate things in that manner for the
        // remote VNC server. However that means we lose the ability to use
        // Alt as a shortcut modifier. Do what RealVNC does and hijack the
        // left command key as an Alt replacement.
          ucs = vk_to_ascii(keyCode, ev.isShiftDown());
    }

    // Dead keys are represented by their spacing equivalent
    // (or something similar depending on the layout)
    if (Character.getType(ucs) == Character.COMBINING_SPACING_MARK)
      return Keysym2ucs.ucs2keysym(Keysym2ucs.ucs2combining(ucs));

    if (Character.isDefined(ucs))
      return Keysym2ucs.ucs2keysym(ucs);

    return NoSymbol;
  }

  public static int get_keycode_fallback_extended(final KeyEvent ev) {
    final int keyCode = ev.getKeyCode();
    return (keyCode == 0) ? ev.getExtendedKeyCode() : keyCode;
  }

  private static int vk_to_ascii(int vk, boolean shift) {
    char c = 0;
    if (code_map_java_to_char.containsKey(vk))
      c = code_map_java_to_char.get(vk);
    // 0x25 (%) and 0x3F (?) do not have VK_ constants
    if (vk == VK_5)
      c = shift ? '%' : c;
    else if (vk == VK_SLASH)
      c = shift ? '?' : c;
    if (Character.isLetter(c))
      c = shift ? Character.toUpperCase(c) : Character.toLowerCase(c);
    return (int)c;
  }

  static LogWriter vlog = new LogWriter("KeyMap");
}
