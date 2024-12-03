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
#ifndef _VNCEXT_H_
#define _VNCEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define X_VncExtSetParam 0
#define X_VncExtGetParam 1
#define X_VncExtGetParamDesc 2
#define X_VncExtListParams 3
#define X_VncExtSelectInput 6
#define X_VncExtConnect 7
#define X_VncExtGetQueryConnect 8
#define X_VncExtApproveConnect 9

#define VncExtQueryConnectNotify 2
#define VncExtQueryConnectMask (1 << VncExtQueryConnectNotify)

#define VncExtNumberEvents 3
#define VncExtNumberErrors 0

#ifndef _VNCEXT_SERVER_

Bool XVncExtQueryExtension(Display* dpy, int* event_basep, int* error_basep);
Bool XVncExtSetParam(Display* dpy, const char* param, const char* value);
Bool XVncExtGetParam(Display* dpy, const char* param, char** value, int* len);
char* XVncExtGetParamDesc(Display* dpy, const char* param);
char** XVncExtListParams(Display* dpy, int* nParams);
void XVncExtFreeParamList(char** list);
Bool XVncExtSelectInput(Display* dpy, Window w, int mask);
Bool XVncExtConnect(Display* dpy, const char* hostAndPort, Bool viewOnly);
Bool XVncExtGetQueryConnect(Display* dpy, char** addr,
                            char** user, int* timeout, void** opaqueId);
Bool XVncExtApproveConnect(Display* dpy, void* opaqueId, int approve);


typedef struct {
  int type;
  unsigned long serial;
  Bool send_event;
  Display *display;
  Window window;
} XVncExtQueryConnectEvent;

#endif

#ifdef _VNCEXT_PROTO_

#define VNCEXTNAME "TIGERVNC"

typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtSetParam */
  CARD16 length;
  CARD16 paramLen;
  CARD16 valueLen;
} xVncExtSetParamReq;
#define sz_xVncExtSetParamReq 8

typedef struct {
 BYTE type; /* X_Reply */
 BYTE success;
 CARD16 sequenceNumber;
 CARD32 length;
 CARD32 pad0;
 CARD32 pad1;
 CARD32 pad2;
 CARD32 pad3;
 CARD32 pad4;
 CARD32 pad5;
} xVncExtSetParamReply;
#define sz_xVncExtSetParamReply 32


typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtGetParam */
  CARD16 length;
  CARD8 paramLen;
  CARD8 pad0;
  CARD16 pad1;
} xVncExtGetParamReq;
#define sz_xVncExtGetParamReq 8

typedef struct {
 BYTE type; /* X_Reply */
 BYTE success;
 CARD16 sequenceNumber;
 CARD32 length;
 CARD16 valueLen;
 CARD16 pad0;
 CARD32 pad1;
 CARD32 pad2;
 CARD32 pad3;
 CARD32 pad4;
 CARD32 pad5;
} xVncExtGetParamReply;
#define sz_xVncExtGetParamReply 32


typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtGetParamDesc */
  CARD16 length;
  CARD8 paramLen;
  CARD8 pad0;
  CARD16 pad1;
} xVncExtGetParamDescReq;
#define sz_xVncExtGetParamDescReq 8

typedef struct {
 BYTE type; /* X_Reply */
 BYTE success;
 CARD16 sequenceNumber;
 CARD32 length;
 CARD16 descLen;
 CARD16 pad0;
 CARD32 pad1;
 CARD32 pad2;
 CARD32 pad3;
 CARD32 pad4;
 CARD32 pad5;
} xVncExtGetParamDescReply;
#define sz_xVncExtGetParamDescReply 32


typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtListParams */
  CARD16 length;
} xVncExtListParamsReq;
#define sz_xVncExtListParamsReq 4

typedef struct {
 BYTE type; /* X_Reply */
 BYTE pad0;
 CARD16 sequenceNumber;
 CARD32 length;
 CARD16 nParams;
 CARD16 pad1;
 CARD32 pad2;
 CARD32 pad3;
 CARD32 pad4;
 CARD32 pad5;
 CARD32 pad6;
} xVncExtListParamsReply;
#define sz_xVncExtListParamsReply 32


typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtSelectInput */
  CARD16 length;
  CARD32 window;
  CARD32 mask;
} xVncExtSelectInputReq;
#define sz_xVncExtSelectInputReq 12


typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtConnect */
  CARD16 length;
  CARD8 strLen;
  CARD8 viewOnly;
  CARD16 pad1;
} xVncExtConnectReq;
#define sz_xVncExtConnectReq 8

typedef struct {
 BYTE type; /* X_Reply */
 BYTE success;
 CARD16 sequenceNumber;
 CARD32 length;
 CARD32 pad0;
 CARD32 pad1;
 CARD32 pad2;
 CARD32 pad3;
 CARD32 pad4;
 CARD32 pad5;
} xVncExtConnectReply;
#define sz_xVncExtConnectReply 32


typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtGetQueryConnect */
  CARD16 length;
} xVncExtGetQueryConnectReq;
#define sz_xVncExtGetQueryConnectReq 4

typedef struct {
 BYTE type; /* X_Reply */
 BYTE pad0;
 CARD16 sequenceNumber;
 CARD32 length;
 CARD32 addrLen;
 CARD32 userLen;
 CARD32 timeout;
 CARD32 opaqueId;
 CARD32 pad4;
 CARD32 pad5;
} xVncExtGetQueryConnectReply;
#define sz_xVncExtGetQueryConnectReply 32

typedef struct {
  CARD8 reqType;       /* always VncExtReqCode */
  CARD8 vncExtReqType; /* always VncExtApproveConnect */
  CARD16 length;
  CARD8 approve;
  CARD8 pad0;
  CARD16 pad1;
  CARD32 opaqueId;
} xVncExtApproveConnectReq;
#define sz_xVncExtApproveConnectReq 12



typedef struct {
  BYTE type;    /* always eventBase + VncExtQueryConnectNotify */
  BYTE pad0;
  CARD16 sequenceNumber;
  CARD32 window;
  CARD32 pad6;
  CARD32 pad1;
  CARD32 pad2;
  CARD32 pad3;
  CARD32 pad4;
  CARD32 pad5;
} xVncExtQueryConnectNotifyEvent;
#define sz_xVncExtQueryConnectNotifyEvent 32

#endif

#ifdef __cplusplus
}
#endif

#endif
