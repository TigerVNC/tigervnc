/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <stdio.h>
#include <stdint.h>

#define NEED_REPLIES
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#define _VNCEXT_PROTO_
#include "vncExt.h"

static Bool XVncExtQueryConnectNotifyWireToEvent(Display* dpy, XEvent* e,
                                                 xEvent* w);

static Bool extensionInited = False;
static XExtCodes* codes = 0;

static Bool checkExtension(Display* dpy)
{
  if (!extensionInited) {
    extensionInited = True;
    codes = XInitExtension(dpy, VNCEXTNAME);
    if (!codes) return False;
    XESetWireToEvent(dpy, codes->first_event + VncExtQueryConnectNotify,
                     XVncExtQueryConnectNotifyWireToEvent);
  }
  return codes != 0;
}

Bool XVncExtQueryExtension(Display* dpy, int* event_basep, int* error_basep)
{
  if (!checkExtension(dpy)) return False;
  *event_basep = codes->first_event;
  *error_basep = codes->first_error;
  return True;
}

Bool XVncExtSetParam(Display* dpy, const char* param)
{
  xVncExtSetParamReq* req;
  xVncExtSetParamReply rep;

  int paramLen = strlen(param);
  if (paramLen > 255) return False;
  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtSetParam, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtSetParam;
  req->length += (paramLen + 3) >> 2;
  req->paramLen = paramLen;
  Data(dpy, param, paramLen);
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  UnlockDisplay(dpy);
  SyncHandle();
  return rep.success;
}

Bool XVncExtGetParam(Display* dpy, const char* param, char** value, int* len)
{
  xVncExtGetParamReq* req;
  xVncExtGetParamReply rep;

  int paramLen = strlen(param);
  *value = 0;
  *len = 0;
  if (paramLen > 255) return False;
  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtGetParam, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtGetParam;
  req->length += (paramLen + 3) >> 2;
  req->paramLen = paramLen;
  Data(dpy, param, paramLen);
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  if (rep.success) {
    *len = rep.valueLen;
    *value = (char*) Xmalloc (*len+1);
    if (!*value) {
      _XEatData(dpy, (*len+1)&~1);
      return False;
    }
    _XReadPad(dpy, *value, *len);
    (*value)[*len] = 0;
  }
  UnlockDisplay(dpy);
  SyncHandle();
  return rep.success;
}

char* XVncExtGetParamDesc(Display* dpy, const char* param)
{
  xVncExtGetParamDescReq* req;
  xVncExtGetParamDescReply rep;
  char* desc = 0;

  int paramLen = strlen(param);
  if (paramLen > 255) return False;
  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtGetParamDesc, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtGetParamDesc;
  req->length += (paramLen + 3) >> 2;
  req->paramLen = paramLen;
  Data(dpy, param, paramLen);
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  if (rep.success) {
    desc = (char*)Xmalloc(rep.descLen+1);
    if (!desc) {
      _XEatData(dpy, (rep.descLen+1)&~1);
      return False;
    }
    _XReadPad(dpy, desc, rep.descLen);
    desc[rep.descLen] = 0;
  }
  UnlockDisplay(dpy);
  SyncHandle();
  return desc;
}

char** XVncExtListParams(Display* dpy, int* nParams)
{
  xVncExtListParamsReq* req;
  xVncExtListParamsReply rep;
  char** list = 0;
  char* ch;
  int rlen, paramLen, i;

  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtListParams, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtListParams;
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  UnlockDisplay(dpy);
  SyncHandle();
  if (rep.nParams) {
    list = (char**)Xmalloc(rep.nParams * sizeof(char*));
    rlen = rep.length << 2;
    ch = (char*)Xmalloc(rlen + 1);
    if (!list || !ch) {
      if (list) Xfree((char*)list);
      if (ch) Xfree(ch);
      _XEatData(dpy, rlen);
      UnlockDisplay(dpy);
      SyncHandle();
      return 0;
    }
    _XReadPad(dpy, ch, rlen);
    paramLen = *ch++;
    for (i = 0; i < rep.nParams; i++) {
      list[i] = ch;
      ch += paramLen;
      paramLen = *ch;
      *ch++ = 0;
    }
  }
  *nParams = rep.nParams;
  UnlockDisplay(dpy);
  SyncHandle();
  return list;
}

void XVncExtFreeParamList(char** list)
{
  if (list) {
    Xfree(list[0]-1);
    Xfree((char*)list);
  }
}

Bool XVncExtSelectInput(Display* dpy, Window w, int mask)
{
  xVncExtSelectInputReq* req;

  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtSelectInput, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtSelectInput;
  req->window = w;
  req->mask = mask;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}

Bool XVncExtConnect(Display* dpy, const char* hostAndPort)
{
  xVncExtConnectReq* req;
  xVncExtConnectReply rep;

  int strLen = strlen(hostAndPort);
  if (strLen > 255) return False;
  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtConnect, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtConnect;
  req->length += (strLen + 3) >> 2;
  req->strLen = strLen;
  Data(dpy, hostAndPort, strLen);
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  UnlockDisplay(dpy);
  SyncHandle();
  return rep.success;
}

Bool XVncExtGetQueryConnect(Display* dpy, char** addr, char** user,
                            int* timeout, void** opaqueId)
{
  xVncExtGetQueryConnectReq* req;
  xVncExtGetQueryConnectReply rep;

  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtGetQueryConnect, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtGetQueryConnect;
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
    UnlockDisplay(dpy);
    SyncHandle();
    return False;
  }
  UnlockDisplay(dpy);
  SyncHandle();

  *addr = Xmalloc(rep.addrLen+1);
  *user = Xmalloc(rep.userLen+1);
  if (!*addr || !*user) {
    Xfree(*addr);
    Xfree(*user);
    _XEatData(dpy, ((rep.addrLen+1)&~1) + ((rep.userLen+1)&~1));
    return False;
  }
  _XReadPad(dpy, *addr, rep.addrLen);
  (*addr)[rep.addrLen] = 0;
  _XReadPad(dpy, *user, rep.userLen);
  (*user)[rep.userLen] = 0;
  *timeout = rep.timeout;
  *opaqueId = (void*)(intptr_t)rep.opaqueId;
  return True;
}

Bool XVncExtApproveConnect(Display* dpy, void* opaqueId, int approve)
{
  xVncExtApproveConnectReq* req;

  if (!checkExtension(dpy)) return False;

  LockDisplay(dpy);
  GetReq(VncExtApproveConnect, req);
  req->reqType = codes->major_opcode;
  req->vncExtReqType = X_VncExtApproveConnect;
  req->approve = approve;
  req->opaqueId = (CARD32)(intptr_t)opaqueId;
  UnlockDisplay(dpy);
  SyncHandle();
  return True;
}


static Bool XVncExtQueryConnectNotifyWireToEvent(Display* dpy, XEvent* e,
                                                    xEvent* w)
{
  XVncExtQueryConnectEvent* ev = (XVncExtQueryConnectEvent*)e;
  xVncExtQueryConnectNotifyEvent* wire
    = (xVncExtQueryConnectNotifyEvent*)w;
  ev->type = wire->type & 0x7f;
  ev->serial = _XSetLastRequestRead(dpy,(xGenericReply*)wire);
  ev->send_event = (wire->type & 0x80) != 0;
  ev->display = dpy;
  ev->window = wire->window;
  return True;
}
