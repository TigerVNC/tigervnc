/* Copyright 2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2012 Brian P. Hinz
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

public class menukey 
{
  static class MenuKeySymbol {
    public MenuKeySymbol(String name_, int keycode_) {
      name = name_;
      keycode = keycode_;
    }
    String name;
    int keycode;
  }

  private static final MenuKeySymbol[] menuSymbols = {
    new MenuKeySymbol("F1", KeyEvent.VK_F1),
    new MenuKeySymbol("F2", KeyEvent.VK_F2),
    new MenuKeySymbol("F3", KeyEvent.VK_F3),
    new MenuKeySymbol("F4", KeyEvent.VK_F4),
    new MenuKeySymbol("F5", KeyEvent.VK_F5),
    new MenuKeySymbol("F6", KeyEvent.VK_F6),
    new MenuKeySymbol("F7", KeyEvent.VK_F7),
    new MenuKeySymbol("F8", KeyEvent.VK_F8),
    new MenuKeySymbol("F9", KeyEvent.VK_F9),
    new MenuKeySymbol("F10", KeyEvent.VK_F10),
    new MenuKeySymbol("F11", KeyEvent.VK_F11),
    new MenuKeySymbol("F12", KeyEvent.VK_F12),
    new MenuKeySymbol("Pause", KeyEvent.VK_PAUSE),
    new MenuKeySymbol("Print", KeyEvent.VK_PRINTSCREEN),
    new MenuKeySymbol("Scroll_Lock", KeyEvent.VK_SCROLL_LOCK),
    new MenuKeySymbol("Escape", KeyEvent.VK_ESCAPE),
    new MenuKeySymbol("Insert", KeyEvent.VK_INSERT),
    new MenuKeySymbol("Delete", KeyEvent.VK_DELETE),
    new MenuKeySymbol("Home", KeyEvent.VK_HOME),
    new MenuKeySymbol("Page_Up", KeyEvent.VK_PAGE_UP),
    new MenuKeySymbol("Page_Down", KeyEvent.VK_PAGE_DOWN)
  };
  
  static int getMenuKeySymbolCount()
  {
    return menuSymbols.length;
  }
  
  public static MenuKeySymbol[] getMenuKeySymbols()
  {
    return menuSymbols;
  }
  
  static int getMenuKeyCode()
  {
      String menuKeyStr;
      int menuKeyCode = KeyEvent.VK_F8;
  
      menuKeyStr = 
        Configuration.global().getParam("menuKey").getValueStr();
      for(int i = 0; i < getMenuKeySymbolCount(); i++)
        if (menuSymbols[i].name.equals(menuKeyStr))
          menuKeyCode = menuSymbols[i].keycode;
  
      return menuKeyCode;
  }

}
