/* Copyright (C) 2024 Gaurav Ujjwal.  All Rights Reserved.
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

#ifndef __XSELECTION_H__
#define __XSELECTION_H__

#include <string>
#include <tx/TXWindow.h>

class XSelectionHandler
{
public:
  virtual void handleXSelectionAnnounce(bool available) = 0;
  virtual void handleXSelectionData(const char* data) = 0;
};

class XSelection : TXWindow
{
public:
  XSelection(Display* dpy_, XSelectionHandler* handler_);

  void handleSelectionOwnerChange(Window owner, Atom selection, Time time);
  void requestSelectionData();
  void handleClientClipboardData(const char* data);

private:
  XSelectionHandler* handler;
  Atom probeProperty;
  Atom transferProperty;
  Atom timestampProperty;
  Atom announcedSelection;
  std::string clientData; // Always in UTF-8

  Time getXServerTime();
  void announceSelection(Atom selection);

  bool selectionRequest(Window requestor, Atom selection, Atom target,
                        Atom property) override;
  void selectionNotify(XSelectionEvent* ev, Atom type, int format, int nitems,
                       void* data) override;
};

#endif
