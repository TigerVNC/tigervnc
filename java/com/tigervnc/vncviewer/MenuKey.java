/* Copyright 2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2012-2017 Brian P. Hinz
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

package com.tigervnc.vncviewer;

import java.awt.event.KeyEvent;

import com.tigervnc.rfb.*;

import static java.awt.event.KeyEvent.*;
import static com.tigervnc.rfb.Keysymdef.*;

public class MenuKey
{
  static class MenuKeySymbol {
    public MenuKeySymbol(String name_, int javacode_,
                         int keycode_, int keysym_) {
      name = name_;
      javacode = javacode_;
      keycode = keycode_;
      keysym = keysym_;
    }
    String name;
    int javacode;
    int keycode;
    int keysym;
  }

  private static final MenuKeySymbol[] menuSymbols = {
    new MenuKeySymbol("F1", VK_F1, 0x3b, XK_F1),
    new MenuKeySymbol("F2", VK_F2, 0x3c, XK_F2),
    new MenuKeySymbol("F3", VK_F3, 0x3d, XK_F3),
    new MenuKeySymbol("F4", VK_F4, 0x3e, XK_F4),
    new MenuKeySymbol("F5", VK_F5, 0x3f, XK_F5),
    new MenuKeySymbol("F6", VK_F6, 0x40, XK_F6),
    new MenuKeySymbol("F7", VK_F7, 0x41, XK_F7),
    new MenuKeySymbol("F8", VK_F8, 0x42, XK_F8),
    new MenuKeySymbol("F9", VK_F9, 0x43, XK_F9),
    new MenuKeySymbol("F10", VK_F10, 0x44, XK_F10),
    new MenuKeySymbol("F11", VK_F11, 0x57, XK_F11),
    new MenuKeySymbol("F12", VK_F12, 0x58, XK_F12),
    new MenuKeySymbol("Pause", VK_PAUSE, 0xc6, XK_Pause),
    new MenuKeySymbol("Scroll_Lock", VK_SCROLL_LOCK,
                      0x46, XK_Scroll_Lock),
    new MenuKeySymbol("Escape", VK_ESCAPE, 0x01, XK_Escape),
    new MenuKeySymbol("Insert", VK_INSERT, 0xd2, XK_Insert),
    new MenuKeySymbol("Delete", VK_DELETE, 0xd3, XK_Delete),
    new MenuKeySymbol("Home", VK_HOME, 0xc7, XK_Home),
    new MenuKeySymbol("Page_Up", VK_PAGE_UP, 0xc9, XK_Page_Up),
    new MenuKeySymbol("Page_Down", VK_PAGE_DOWN, 0xd1, XK_Page_Down)
  };

  static int getMenuKeySymbolCount() {
    return menuSymbols.length;
  }

  public static MenuKeySymbol[] getMenuKeySymbols() {
    return menuSymbols;
  }

  public static String getKeyText(MenuKeySymbol sym) {
    if (VncViewer.os.startsWith("mac os x"))
      return sym.name.replace("_", " ");
    else
      return KeyEvent.getKeyText(sym.javacode);
  }

  public static String getMenuKeyValueStr() {
    String s = "";
    for (int i = 0; i < getMenuKeySymbolCount(); i++) {
      s += menuSymbols[i].name;
      if (i < getMenuKeySymbolCount() - 1)
        s += ", ";
    }
    return s;
  }

  static int getMenuKeyJavaCode() {
    int menuKeyCode = VK_F8;

    @SuppressWarnings({"static"})
    String menuKeyStr =
      Configuration.global().getParam("menuKey").getValueStr();
    for(int i = 0; i < getMenuKeySymbolCount(); i++)
      if (menuSymbols[i].name.equals(menuKeyStr))
        menuKeyCode = menuSymbols[i].javacode;

    return menuKeyCode;
  }

  static int getMenuKeyCode() {
    int menuKeyCode = 0x42;

    @SuppressWarnings({"static"})
    String menuKeyStr =
      Configuration.global().getParam("menuKey").getValueStr();
    for(int i = 0; i < getMenuKeySymbolCount(); i++)
      if (menuSymbols[i].name.equals(menuKeyStr))
        menuKeyCode = menuSymbols[i].keycode;

    return menuKeyCode;
  }

  static int getMenuKeySym() {
    int menuKeySym = XK_F8;

    @SuppressWarnings({"static"})
    String menuKeyStr =
      Configuration.global().getParam("menuKey").getValueStr();
    for(int i = 0; i < getMenuKeySymbolCount(); i++)
      if (menuSymbols[i].name.equals(menuKeyStr))
        menuKeySym = menuSymbols[i].keysym;

    return menuKeySym;
  }

}
