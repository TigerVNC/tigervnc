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

#include <X11/Xatom.h>

#include <core/Configuration.h>
#include <core/LogWriter.h>
#include <core/string.h>

#include <x0vncserver/XSelection.h>

core::BoolParameter
  setPrimary("SetPrimary",
             "Set the PRIMARY as well as the CLIPBOARD selection",
             true);
core::BoolParameter
  sendPrimary("SendPrimary",
              "Send the PRIMARY as well as the CLIPBOARD selection",
              true);

static core::LogWriter vlog("XSelection");

XSelection::XSelection(Display* dpy_, XSelectionHandler* handler_)
    : TXWindow(dpy_, 1, 1, nullptr), handler(handler_), announcedSelection(None)
{
  probeProperty = XInternAtom(dpy, "TigerVNC_ProbeProperty", False);
  transferProperty = XInternAtom(dpy, "TigerVNC_TransferProperty", False);
  timestampProperty = XInternAtom(dpy, "TigerVNC_TimestampProperty", False);
  toplevel("TigerVNC Clipboard (x0vncserver)");
  addEventMask(PropertyChangeMask); // Required for PropertyNotify events
}

static Bool PropertyEventMatcher(Display* /* dpy */, XEvent* ev, XPointer prop)
{
  if (ev->type == PropertyNotify && ev->xproperty.atom == *((Atom*)prop))
    return True;
  else
    return False;
}

Time XSelection::getXServerTime()
{
  XEvent ev;
  uint8_t data = 0;

  // Trigger a PropertyNotify event to extract server time
  XChangeProperty(dpy, win(), timestampProperty, XA_STRING, 8, PropModeReplace,
                  &data, sizeof(data));
  XIfEvent(dpy, &ev, &PropertyEventMatcher, (XPointer)&timestampProperty);
  return ev.xproperty.time;
}

// Takes ownership of selections, backed by given data.
void XSelection::handleClientClipboardData(const char* data)
{
  vlog.debug("Received client clipboard data, taking selection ownership");

  Time time = getXServerTime();
  ownSelection(xaCLIPBOARD, time);
  if (!selectionOwner(xaCLIPBOARD))
    vlog.error("Unable to own CLIPBOARD selection");

  if (setPrimary) {
    ownSelection(XA_PRIMARY, time);
    if (!selectionOwner(XA_PRIMARY))
      vlog.error("Unable to own PRIMARY selection");
  }

  if (selectionOwner(xaCLIPBOARD) || selectionOwner(XA_PRIMARY))
    clientData = data;
}

// We own the selection and another X app has asked for data
bool XSelection::selectionRequest(Window requestor, Atom selection, Atom target,
                                  Atom property)
{
  if (clientData.empty() || requestor == win() || !selectionOwner(selection))
    return false;

  if (target == XA_STRING) {
    std::string latin1 = core::utf8ToLatin1(clientData.data(), clientData.length());
    XChangeProperty(dpy, requestor, property, XA_STRING, 8, PropModeReplace,
                    (unsigned char*)latin1.data(), latin1.length());
    return true;
  }

  if (target == xaUTF8_STRING) {
    XChangeProperty(dpy, requestor, property, xaUTF8_STRING, 8, PropModeReplace,
                    (unsigned char*)clientData.data(), clientData.length());
    return true;
  }

  return false;
}

// Selection-owner change implies a change in selection data.
void XSelection::handleSelectionOwnerChange(Window owner, Atom selection, Time time)
{
  if (selection != XA_PRIMARY && selection != xaCLIPBOARD)
    return;
  if (selection == XA_PRIMARY && !sendPrimary)
    return;

  if (selection == announcedSelection)
    announceSelection(None);

  if (owner == None || owner == win())
    return;

  if (!selectionOwner(XA_PRIMARY) && !selectionOwner(xaCLIPBOARD))
    clientData = "";

  XConvertSelection(dpy, selection, xaTARGETS, probeProperty, win(), time);
}

void XSelection::announceSelection(Atom selection)
{
  announcedSelection = selection;
  handler->handleXSelectionAnnounce(selection != None);
}

void XSelection::requestSelectionData()
{
  if (announcedSelection != None)
    XConvertSelection(dpy, announcedSelection, xaTARGETS, transferProperty, win(),
                      CurrentTime);
}

// Some information about selection is received from current owner
void XSelection::selectionNotify(XSelectionEvent* ev, Atom type, int format,
                                 int nitems, void* data)
{
  if (!ev || !data || type == None)
    return;

  if (ev->target == xaTARGETS) {
    if (format != 32 || type != XA_ATOM)
      return;

    Atom* targets = (Atom*)data;
    bool utf8Supported = false;
    bool stringSupported = false;

    for (int i = 0; i < nitems; i++) {
      if (targets[i] == xaUTF8_STRING)
        utf8Supported = true;
      else if (targets[i] == XA_STRING)
        stringSupported = true;
    }

    if (ev->property == probeProperty) {
      // Only probing for now, will issue real request when client asks for data
      if (stringSupported || utf8Supported)
        announceSelection(ev->selection);
      return;
    }

    // Prefer UTF-8 if available
    if (utf8Supported)
      XConvertSelection(dpy, ev->selection, xaUTF8_STRING, transferProperty, win(),
                        ev->time);
    else if (stringSupported)
      XConvertSelection(dpy, ev->selection, XA_STRING, transferProperty, win(),
                        ev->time);
  } else if (ev->target == xaUTF8_STRING || ev->target == XA_STRING) {
    if (type == xaINCR) {
      // Incremental transfer is not supported
      vlog.debug("Selected data is too big!");
      return;
    }

    if (format != 8)
      return;

    if (type == xaUTF8_STRING) {
      std::string result = core::convertLF((char*)data, nitems);
      handler->handleXSelectionData(result.c_str());
    } else if (type == XA_STRING) {
      std::string result = core::convertLF((char*)data, nitems);
      result = core::latin1ToUTF8(result.data(), result.length());
      handler->handleXSelectionData(result.c_str());
    }
  }
}