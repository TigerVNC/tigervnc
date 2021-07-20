/* Copyright 2016-2019 Pierre Ossman for Cendio AB
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
static Atom xaTARGETS, xaTIMESTAMP, xaSTRING, xaTEXT, xaUTF8_STRING;

static WindowPtr pWindow;
static Window wid;

static Bool probing;
static Atom activeSelection = None;

static char* cachedData = NULL;

struct VncDataTarget {
  ClientPtr client;
  Atom selection;
  Atom target;
  Atom property;
  Window requestor;
  CARD32 time;
  struct VncDataTarget* next;
};

static struct VncDataTarget* vncDataTargetHead;

static int vncCreateSelectionWindow(void);
static int vncOwnSelection(Atom selection);
static int vncConvertSelection(ClientPtr client, Atom selection,
                               Atom target, Atom property,
                               Window requestor, CARD32 time,
                               const char* data);
static int vncProcConvertSelection(ClientPtr client);
static void vncSelectionRequest(Atom selection, Atom target);
static int vncProcSendEvent(ClientPtr client);
static void vncSelectionCallback(CallbackListPtr *callbacks,
                                 void * data, void * args);
static void vncClientStateCallback(CallbackListPtr * l,
                                   void * d, void * p);

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
  xaUTF8_STRING = MakeAtom("UTF8_STRING", 11, TRUE);

  /* There are no hooks for when these are internal windows, so
   * override the relevant handlers. */
  origProcConvertSelection = ProcVector[X_ConvertSelection];
  ProcVector[X_ConvertSelection] = vncProcConvertSelection;
  origProcSendEvent = ProcVector[X_SendEvent];
  ProcVector[X_SendEvent] = vncProcSendEvent;

  if (!AddCallback(&SelectionCallback, vncSelectionCallback, 0))
    FatalError("Add VNC SelectionCallback failed\n");
  if (!AddCallback(&ClientStateCallback, vncClientStateCallback, 0))
    FatalError("Add VNC ClientStateCallback failed\n");
}

void vncHandleClipboardRequest(void)
{
  if (activeSelection == None) {
    LOG_DEBUG("Got request for local clipboard although no clipboard is active");
    return;
  }

  LOG_DEBUG("Got request for local clipboard, re-probing formats");

  probing = FALSE;
  vncSelectionRequest(activeSelection, xaTARGETS);
}

void vncHandleClipboardAnnounce(int available)
{
  /* The data has changed in some way, so whatever is in our cache is
   * now stale */
  free(cachedData);
  cachedData = NULL;

  if (available) {
    int rc;

    LOG_DEBUG("Remote clipboard announced, grabbing local ownership");

    if (vncGetSetPrimary()) {
      rc = vncOwnSelection(xaPRIMARY);
      if (rc != Success)
        LOG_ERROR("Could not set PRIMARY selection");
    }

    rc = vncOwnSelection(xaCLIPBOARD);
    if (rc != Success)
      LOG_ERROR("Could not set CLIPBOARD selection");
  } else {
    struct VncDataTarget* next;

    if (pWindow == NULL)
      return;

    LOG_DEBUG("Remote clipboard lost, removing local ownership");

    DeleteWindowFromAnySelections(pWindow);

    /* Abort any pending transfer */
    while (vncDataTargetHead != NULL) {
      xEvent event;

      event.u.u.type = SelectionNotify;
      event.u.selectionNotify.time = vncDataTargetHead->time;
      event.u.selectionNotify.requestor = vncDataTargetHead->requestor;
      event.u.selectionNotify.selection = vncDataTargetHead->selection;
      event.u.selectionNotify.target = vncDataTargetHead->target;
      event.u.selectionNotify.property = None;
      WriteEventsToClient(vncDataTargetHead->client, 1, &event);

      next = vncDataTargetHead->next;
      free(vncDataTargetHead);
      vncDataTargetHead = next;
    }
  }
}

void vncHandleClipboardData(const char* data)
{
  struct VncDataTarget* next;

  LOG_DEBUG("Got remote clipboard data, sending to X11 clients");

  free(cachedData);
  cachedData = strdup(data);

  while (vncDataTargetHead != NULL) {
    int rc;
    xEvent event;

    rc = vncConvertSelection(vncDataTargetHead->client,
                             vncDataTargetHead->selection,
                             vncDataTargetHead->target,
                             vncDataTargetHead->property,
                             vncDataTargetHead->requestor,
                             vncDataTargetHead->time,
                             cachedData);
    if (rc != Success) {
      event.u.u.type = SelectionNotify;
      event.u.selectionNotify.time = vncDataTargetHead->time;
      event.u.selectionNotify.requestor = vncDataTargetHead->requestor;
      event.u.selectionNotify.selection = vncDataTargetHead->selection;
      event.u.selectionNotify.target = vncDataTargetHead->target;
      event.u.selectionNotify.property = None;
      WriteEventsToClient(vncDataTargetHead->client, 1, &event);
    }

    next = vncDataTargetHead->next;
    free(vncDataTargetHead);
    vncDataTargetHead = next;
  }
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

static Bool vncWeAreOwner(Atom selection)
{
  Selection *pSel;
  int rc;

  rc = dixLookupSelection(&pSel, selection, serverClient, DixReadAccess);
  if (rc != Success)
    return FALSE;

  if (pSel->client != serverClient)
    return FALSE;

  if (pSel->window != wid)
    return FALSE;

  return TRUE;
}

static void vncMaybeRequestCache(void)
{
  /* Telling a client that we have clipboard data will likely mean that
   * we can no longer request its clipboard data. This is a problem as
   * we might initially own multiple selections and we now just lost
   * one, and we still want to be able to service the other one. Solve
   * this by requesting the data from the client when we can't affort to
   * lose it and cache it. */

  /* Already cached? */
  if (cachedData != NULL)
    return;

  if (!vncWeAreOwner(xaCLIPBOARD)) {
    if (!vncGetSetPrimary())
      return;
    if (!vncWeAreOwner(xaPRIMARY))
      return;
  }

  LOG_DEBUG("Requesting clipboard data from client for caching");

  vncRequestClipboard();
}

static int vncConvertSelection(ClientPtr client, Atom selection,
                               Atom target, Atom property,
                               Window requestor, CARD32 time,
                               const char* data)
{
  Selection *pSel;
  WindowPtr pWin;
  int rc;

  Atom realProperty;

  xEvent event;

  if (data == NULL) {
    LOG_DEBUG("Selection request for %s (type %s)",
              NameForAtom(selection), NameForAtom(target));
  } else {
    LOG_DEBUG("Sending data for selection request for %s (type %s)",
              NameForAtom(selection), NameForAtom(target));
  }

  rc = dixLookupSelection(&pSel, selection, client, DixGetAttrAccess);
  if (rc != Success)
    return rc;

  /* We do not validate the time argument because neither does
   * dix/selection.c and some clients (e.g. Qt) relies on this */

  rc = dixLookupWindow(&pWin, requestor, client, DixSetAttrAccess);
  if (rc != Success)
    return rc;

  if (property != None)
    realProperty = property;
  else
    realProperty = target;

  /* FIXME: MULTIPLE target */

  if (target == xaTARGETS) {
    Atom targets[] = { xaTARGETS, xaTIMESTAMP,
                       xaSTRING, xaTEXT, xaUTF8_STRING };

    rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                 XA_ATOM, 32, PropModeReplace,
                                 sizeof(targets)/sizeof(targets[0]),
                                 targets, TRUE);
    if (rc != Success)
      return rc;
  } else if (target == xaTIMESTAMP) {
    rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                 XA_INTEGER, 32, PropModeReplace, 1,
                                 &pSel->lastTimeChanged.milliseconds,
                                 TRUE);
    if (rc != Success)
      return rc;
  } else {
    if (data == NULL) {
      struct VncDataTarget* vdt;

      if ((target != xaSTRING) && (target != xaTEXT) &&
          (target != xaUTF8_STRING))
        return BadMatch;

      vdt = calloc(1, sizeof(struct VncDataTarget));
      if (vdt == NULL)
        return BadAlloc;

      vdt->client = client;
      vdt->selection = selection;
      vdt->target = target;
      vdt->property = property;
      vdt->requestor = requestor;
      vdt->time = time;

      vdt->next = vncDataTargetHead;
      vncDataTargetHead = vdt;

      LOG_DEBUG("Requesting clipboard data from client");

      vncRequestClipboard();

      return Success;
    } else {
      if ((target == xaSTRING) || (target == xaTEXT)) {
        char* latin1;

        latin1 = vncUTF8ToLatin1(data, (size_t)-1);
        if (latin1 == NULL)
          return BadAlloc;

        rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                     XA_STRING, 8, PropModeReplace,
                                     strlen(latin1), latin1, TRUE);

        vncStrFree(latin1);

        if (rc != Success)
          return rc;
      } else if (target == xaUTF8_STRING) {
        rc = dixChangeWindowProperty(serverClient, pWin, realProperty,
                                     xaUTF8_STRING, 8, PropModeReplace,
                                     strlen(data), data, TRUE);
        if (rc != Success)
          return rc;
      } else {
        return BadMatch;
      }
    }
  }

  event.u.u.type = SelectionNotify;
  event.u.selectionNotify.time = time;
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

  /* Do we own this selection? */
  rc = dixLookupSelection(&pSel, stuff->selection, client, DixReadAccess);
  if (rc == Success && pSel->client == serverClient &&
      pSel->window == wid) {
    /* cachedData will be NULL for the first request, but can then be
     * reused once we've gotten the data once from the client */
    rc = vncConvertSelection(client, stuff->selection,
                             stuff->target, stuff->property,
                             stuff->requestor, stuff->time,
                             cachedData);
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

static Bool vncHasAtom(Atom atom, const Atom list[], size_t size)
{
  size_t i;

  for (i = 0;i < size;i++) {
    if (list[i] == atom)
      return TRUE;
  }

  return FALSE;
}

static void vncHandleSelection(Atom selection, Atom target,
                               Atom property, Atom requestor,
                               TimeStamp time)
{
  PropertyPtr prop;
  int rc;

  rc = dixLookupProperty(&prop, pWindow, property,
                         serverClient, DixReadAccess);
  if (rc != Success)
    return;

  LOG_DEBUG("Selection notification for %s (target %s, property %s, type %s)",
            NameForAtom(selection), NameForAtom(target),
            NameForAtom(property), NameForAtom(prop->type));

  if (target != property)
    return;

  if (target == xaTARGETS) {
    if (prop->format != 32)
      return;
    if (prop->type != XA_ATOM)
      return;

    if (probing) {
      if (vncHasAtom(xaSTRING, (const Atom*)prop->data, prop->size) ||
          vncHasAtom(xaUTF8_STRING, (const Atom*)prop->data, prop->size)) {
        vncMaybeRequestCache();
        LOG_DEBUG("Compatible format found, notifying clients");
        activeSelection = selection;
        vncAnnounceClipboard(TRUE);
      }
    } else {
      if (vncHasAtom(xaUTF8_STRING, (const Atom*)prop->data, prop->size))
        vncSelectionRequest(selection, xaUTF8_STRING);
      else if (vncHasAtom(xaSTRING, (const Atom*)prop->data, prop->size))
        vncSelectionRequest(selection, xaSTRING);
    }
  } else if (target == xaSTRING) {
    char* filtered;
    char* utf8;

    if (prop->format != 8)
      return;
    if (prop->type != xaSTRING)
      return;

    filtered = vncConvertLF(prop->data, prop->size);
    if (filtered == NULL)
      return;

    utf8 = vncLatin1ToUTF8(filtered, (size_t)-1);
    vncStrFree(filtered);
    if (utf8 == NULL)
      return;

    LOG_DEBUG("Sending clipboard to clients (%d bytes)",
              (int)strlen(utf8));

    vncSendClipboardData(utf8);

    vncStrFree(utf8);
  } else if (target == xaUTF8_STRING) {
    char *filtered;

    if (prop->format != 8)
      return;
    if (prop->type != xaUTF8_STRING)
      return;

    filtered = vncConvertLF(prop->data, prop->size);
    if (filtered == NULL)
      return;

    LOG_DEBUG("Sending clipboard to clients (%d bytes)",
              (int)strlen(filtered));

    vncSendClipboardData(filtered);

    vncStrFree(filtered);
  }
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

  if (info->selection->selection == activeSelection) {
    vncMaybeRequestCache();
    LOG_DEBUG("Local clipboard lost, notifying clients");
    activeSelection = None;
    vncAnnounceClipboard(FALSE);
  }

  if (info->kind != SelectionSetOwner)
    return;
  if (info->client == serverClient)
    return;

  LOG_DEBUG("Selection owner change for %s",
            NameForAtom(info->selection->selection));

  if ((info->selection->selection != xaPRIMARY) &&
      (info->selection->selection != xaCLIPBOARD))
    return;

  if ((info->selection->selection == xaPRIMARY) &&
      !vncGetSendPrimary())
    return;

  LOG_DEBUG("Got clipboard notification, probing for formats");

  probing = TRUE;
  vncSelectionRequest(info->selection->selection, xaTARGETS);
}

static void vncClientStateCallback(CallbackListPtr * l,
                                   void * d, void * p)
{
  ClientPtr client = ((NewClientInfoRec*)p)->client;
  if (client->clientState == ClientStateGone) {
    struct VncDataTarget** nextPtr = &vncDataTargetHead;
    for (struct VncDataTarget* cur = vncDataTargetHead; cur; cur = *nextPtr) {
      if (cur->client == client) {
        *nextPtr = cur->next;
        free(cur);
        continue;
      }
      nextPtr = &cur->next;
    }
  }
}
