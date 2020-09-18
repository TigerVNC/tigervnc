/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman for Cendio AB
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

#ifndef RFB_GLUE_H
#define RFB_GLUE_H

#ifdef __cplusplus
extern "C" {
#endif

void vncInitRFB(void);

#ifdef __GNUC__
#  define __printf_attr(a, b) __attribute__((__format__ (__printf__, a, b)))
#else
#  define __printf_attr(a, b)
#endif // __GNUC__

void vncLogError(const char *name, const char *format, ...) __printf_attr(2, 3);
void vncLogStatus(const char *name, const char *format, ...) __printf_attr(2, 3);
void vncLogInfo(const char *name, const char *format, ...) __printf_attr(2, 3);
void vncLogDebug(const char *name, const char *format, ...) __printf_attr(2, 3);

int vncSetParam(const char *name, const char *value);
int vncSetParamSimple(const char *nameAndValue);
char* vncGetParam(const char *name);
const char* vncGetParamDesc(const char *name);
int vncIsParamBool(const char *name);

int vncGetParamCount(void);
char *vncGetParamList(void);
void vncListParams(int width, int nameWidth);

int vncGetSocketPort(int fd);
int vncIsTCPPortUsed(int port);

char* vncConvertLF(const char* src, size_t bytes);

char* vncLatin1ToUTF8(const char* src, size_t bytes);
char* vncUTF8ToLatin1(const char* src, size_t bytes);

void vncStrFree(char* str);

#ifdef __cplusplus
}
#endif

#endif
