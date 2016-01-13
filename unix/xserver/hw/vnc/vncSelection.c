/* Copyright 2016 Pierre Ossman for Cendio AB
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xatom.h>

#include "propertyst.h"
#include "scrnintstr.h"
#include "selection.h"
#include "windowstr.h"
#include "xace.h"

#include "xorg-version.h"

#include "vncExtInit.h"
#include "vncSelection.h"
#include "RFBGlue.h"

#define LOG_NAME "Selection"

#define LOG_ERROR(...) vncLogError(LOG_NAME, __VA_ARGS__)
#define LOG_STATUS(...) vncLogStatus(LOG_NAME, __VA_ARGS__)
#define LOG_INFO(...) vncLogInfo(LOG_NAME, __VA_ARGS__)
#define LOG_DEBUG(...) vncLogDebug(LOG_NAME, __VA_ARGS__)

static Atom xaPRIMARY, xaCLIPBOARD;
static Atom xaTARGETS, xaTIMESTAMP, xaSTRING, xaTEXT;

static WindowPtr pWindow;
static Window wid;

static char* clientCutText;
static int clientCutTextLen;

static int vncCreateSelectionWindow(void);
static int vncOwnSelection(Atom selection);
static int vncProcConvertSelection(ClientPtr client);
static int vncProcSendEvent(ClientPtr client);
static void vncSelectionCallback(CallbackListPtr *callbacks,
                                 void * data, void * args);

static int (*origProcConvertSelection)(ClientPtr);
static int (*origProcSendEvent)(ClientPtr);

void vncSelectionInit(void)
{
  xaPRIMARY = MakeAtom("PRIMARY", 7, TRUE);
  xaCLIPBOARD = MakeAtom("CLIPBOARD", 9, TRUE);

  xaTARGETS = MakeAtom("TARGETS", 7, TRUE);
  xaTIMESTAMP = MakeAtom("TIMESTAMP", 9, TRUE);
  xaSTRING = MakeAtom("STRING", 6, TRUE);
  xaTEXT = MakeAtom("TEXT", 4, TRUE);

  /* There are no hooks for when these are internal windows, so
   * override the relevant handlers. */
  origProcConvertSelection = ProcVector[X_ConvertSelection];
  ProcVector[X_ConvertSelection] = vncProcConvertSelection;
  origProcSendEvent = ProcVector[X_SendEvent];
  ProcVector[X_SendEvent] = vncProcSendEvent;

  if (!AddCallback(&SelectionCallback, vncSelectionCallback, 0))
    FatalError("Add VNC SelectionCallback failed\n");
}

void vncClientCutText(const char* str, int len)
{
  int rc;

  if (clientCutText != NULL)
    free(clientCutText);

  clientCutText = malloc(len);
  if (clientCutText == NULL) {
    LOG_ERROR("Could not allocate clipboard buffer");
    DeleteWindowFromAnySelections(pWindow);
    return;
  }

  memcpy(clientCutText, str, len);
  clientCutTextLen = len;

  rc = vncOwnSelection(xaPRIMARY);
  if (rc != Success)
    LOG_ERROR("Could not set PRIMARY selection");
  vncOwnSelection(xaCLIPBOARD);
  if (rc != Success)
    LOG_ERROR("Could not set CLIPBOARD selection");
}

static int vncCreateSelectionWindow(void)
{
  ScreenPtr pScreen;
  int result;

  if (pWindow != NULL)
    return Success;

  pScreen = screenInfo.screens[0];

  wid = FakeClientID(0);
  pWindow = CreateWindow(wid, pScreen->root,
                         0, 0, 100, 100, 0, InputOnly,
                         0, NULL, 0, serverClient,
                         CopyFromParent, &result);
  if (!pWindow)
    return result;

  if (!AddResource(pWindow->drawable.id, RT_WINDOW, pWindow))
      return BadAlloc;

  LOG_DEBUG("Created selection window");

  return Success;
}

static int vncOwnSelection(Atom selection)
{
  Selection *pSel;
  int rc;

  SelectionInfoRec info;

  rc = vncCreateSelectionWindow();
  if (rc != Success)
    return rc;

  rc = dixLookupSelection(&pSel, selection, serverClient, DixSetAttrAccess);
  if (rc == Success) {
    if (pSel->client && (pSel->client != serverClient)) {
      xEvent event = {
        .u.selectionClear.time = currentTime.milliseconds,
        .u.selectionClear.window = pSel->window,
        .u.selectionClear.atom = pSel->selection
      };
      event.u.u.type = SelectionClear;
      WriteEventsToClient(pSel->client, 1, &event);
    }
  } else if (rc == BadMatch) {
    pSel = dixAllocateObjectWithPrivates(Selection, PRIVATE_SELECTION);
    if (!pSel)
      return BadAlloc;

    pSel->selection = selection;

    rc = XaceHookSelectionAccess(serverClient, &pSel,
                                 DixCreateAccess | DixSetAttrAccess);
    if (rc != Success) {
        free(pSel);
        return rc;
    }

    pSel->next = CurrentSelections;
    CurrentSelections = pSel;
  }
  else
      return rc;

  pSel->lastTimeChanged = currentTime;
  pSel->window = wid;
  pSel->pWin = pWindow;
  pSel->client = serverClient;

  LOG_DEBUG("Grabbed %s selection", NameForAtom(selection));

  info.selection = pSel;
  info.client = serverClient;
  info.kind = SelectionSetOwner;
  CallCallbacks(&SelectionCallback, &info);

  return Success;
}

static int vncConvertSelection(ClientPtr client, Atom selection,
                               Atom target, Atom property,
                               Window requestor, TimeStamp time)
{
  Selection *pSel;
  WindowPtr pWin;
  int rc;

  Atom realProperty;

  xEvent event;

  LOG_DEBUG("Selection request for %s (type %s)",
            NameForAtom(selection), NameForAtom(target));

  rc = dixLookupSelection(&pSel, selection, client, DixGetAttrAccess);
  if (rc != Success)
    return rc;

  if (CompareTimeStamps(time, pSel->lastTimeChanged) != LATER)
    return BadMatch;

  rc = dixLookupWindow(&pWin, requestor, client, DixSetAttrAccess);
  if (rc != Success)
    return rc;

  if (property != None)
    realProperty = property;
  else
    realProperty = target;

  /* FIXME: MULTIPLE target */

  if (target == xaTARGETS) {
    Atom targets[] = { xaTARGETS, xaTIMESTAMP, xaSTRING, xaTEXT };

    rc = ChangeWindowProperty(pWin, realProperty, XA_ATOM, 32,
                              PropModeReplace,
                              sizeof(targets)/sizeof(targets[0]),
                              targets, TRUE);
    if (rc != Success)
      return rc;
  } else if (target == xaTIMESTAMP) {
    rc = ChangeWindowProperty(pWin, realProperty, XA_INTEGER, 32,
                              PropModeReplace, 1,
                              &pSel->lastTimeChanged.milliseconds,
                              TRUE);
    if (rc != Success)
      return rc;
  } else if ((target == xaSTRING) || (target == xaTEXT)) {
    rc = ChangeWindowProperty(pWin, realProperty, XA_STRING, 8,
                              PropModeReplace, clientCutTextLen,
                              clientCutText, TRUE);
    if (rc != Success)
      return rc;
  } else {
    return BadMatch;
  }

  event.u.u.type = SelectionNotify;
  event.u.selectionNotify.time = time.milliseconds;
  event.u.selectionNotify.requestor = requestor;
  event.u.selectionNotify.selection = selection;
  event.u.selectionNotify.target = target;
  event.u.selectionNotify.property = property;
  WriteEventsToClient(client, 1, &event);
  return Success;
}

static int vncProcConvertSelection(ClientPtr client)
{
  Bool paramsOkay;
  WindowPtr pWin;
  Selection *pSel;
  int rc;

  REQUEST(xConvertSelectionReq);
  REQUEST_SIZE_MATCH(xConvertSelectionReq);

  rc = dixLookupWindow(&pWin, stuff->requestor, client, DixSetAttrAccess);
  if (rc != Success)
    return rc;

  paramsOkay = ValidAtom(stuff->selection) && ValidAtom(stuff->target);
  paramsOkay &= (stuff->property == None) || ValidAtom(stuff->property);
  if (!paramsOkay) {
    client->errorValue = stuff->property;
    return BadAtom;
  }

  rc = dixLookupSelection(&pSel, stuff->selection, client, DixReadAccess);
  if (rc == Success && pSel->client == serverClient &&
      pSel->window == wid) {
    TimeStamp time;
    time = ClientTimeToServerTime(stuff->time);
    rc = vncConvertSelection(client, stuff->selection,
                             stuff->target, stuff->property,
                             stuff->requestor, time);
    if (rc != Success) {
      xEvent event;

      memset(&event, 0, sizeof(xEvent));
      event.u.u.type = SelectionNotify;
      event.u.selectionNotify.time = stuff->time;
      event.u.selectionNotify.requestor = stuff->requestor;
      event.u.selectionNotify.selection = stuff->selection;
      event.u.selectionNotify.target = stuff->target;
      event.u.selectionNotify.property = None;
      WriteEventsToClient(client, 1, &event);
    }

    return Success;
  }

  return origProcConvertSelection(client);
}

static void vncSelectionRequest(Atom selection, Atom target)
{
  Selection *pSel;
  xEvent event;
  int rc;

  rc = vncCreateSelectionWindow();
  if (rc != Success)
    return;

  LOG_DEBUG("Requesting %s for %s selection",
            NameForAtom(target), NameForAtom(selection));

  rc = dixLookupSelection(&pSel, selection, serverClient, DixGetAttrAccess);
  if (rc != Success)
    return;

  event.u.u.type = SelectionRequest;
  event.u.selectionRequest.owner = pSel->window;
  event.u.selectionRequest.time = currentTime.milliseconds;
  event.u.selectionRequest.requestor = wid;
  event.u.selectionRequest.selection = selection;
  event.u.selectionRequest.target = target;
  event.u.selectionRequest.property = target;
  WriteEventsToClient(pSel->client, 1, &event);
}

static void vncHandleSelection(Atom selection, Atom target,
                               Atom property, Atom requestor,
                               TimeStamp time)
{
  PropertyPtr prop;
  int rc;

  LOG_DEBUG("Selection notification for %s (target %s, property %s)",
            NameForAtom(selection), NameForAtom(target),
            NameForAtom(property));

  if (target != xaSTRING)
    return;
  if (property != xaSTRING)
    return;

  rc = dixLookupProperty(&prop, pWindow, xaSTRING,
                         serverClient, DixReadAccess);
  if (rc != Success)
    return;

  if (prop->type != xaSTRING)
    return;
  if (prop->format != 8)
    return;

  vncServerCutText(prop->data, prop->size);
}

#define SEND_EVENT_BIT 0x80

static int vncProcSendEvent(ClientPtr client)
{
  REQUEST(xSendEventReq);
  REQUEST_SIZE_MATCH(xSendEventReq);

  stuff->event.u.u.type &= ~(SEND_EVENT_BIT);

  if (stuff->event.u.u.type == SelectionNotify &&
      stuff->event.u.selectionNotify.requestor == wid) {
    TimeStamp time;
    time = ClientTimeToServerTime(stuff->event.u.selectionNotify.time);
    vncHandleSelection(stuff->event.u.selectionNotify.selection,
                       stuff->event.u.selectionNotify.target,
                       stuff->event.u.selectionNotify.property,
                       stuff->event.u.selectionNotify.requestor,
                       time);
  }

  return origProcSendEvent(client);
}

static void vncSelectionCallback(CallbackListPtr *callbacks,
                                 void * data, void * args)
{
  SelectionInfoRec *info = (SelectionInfoRec *) args;

  if (info->kind != SelectionSetOwner)
    return;
  if (info->client == serverClient)
    return;

  if ((info->selection->selection != xaPRIMARY) &&
      (info->selection->selection != xaCLIPBOARD))
    return;

  if ((info->selection->selection == xaPRIMARY) &&
      !vncGetSendPrimary())
    return;

  vncSelectionRequest(info->selection->selection, xaSTRING);
}
