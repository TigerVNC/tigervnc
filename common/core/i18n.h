/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2023 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __RFB_I18N_H__
#define __RFB_I18N_H__

#define DEFAULT_TEXT_DOMAIN "tigervnc"

/*
 * LC_MESSAGES is only in POSIX, and hence missing on Windows. libintl
 * fixes that for us, but if that isn't included then we need to sort it
 * out ourselves.
 */
#if !defined ENABLE_NLS || !ENABLE_NLS
#include <locale.h>
#ifndef LC_MESSAGES
#define LC_MESSAGES 1729
#endif
#endif

#include "gettext.h"

/* gettext breaks -Wformat (upstream bug 64384) */
#if defined(__GNUC__) && defined(printf)
extern int fprintf (FILE *, const char *, ...)
  __attribute__((__format__ (__printf__, 2, 3)));
extern int printf (const char *, ...)
  __attribute__((__format__ (__printf__, 1, 2)));
extern int sprintf (char *, const char *, ...)
  __attribute__((__format__ (__printf__, 2, 3)));
extern int snprintf (char *, size_t, const char *, ...)
  __attribute__((__format__ (__printf__, 3, 4)));
extern int asprintf (char **, const char *, ...)
  __attribute__((__format__ (__printf__, 2, 3)));
#endif
#if defined(__GNUC__) && defined(wprintf)
extern int fwprintf (FILE *, const wchar_t *, ...)
  /* __attribute__((__format__ (__wprintf__, 2, 3))) */;
extern int wprintf (const wchar_t *, ...)
  /* __attribute__((__format__ (__wprintf__, 1, 2))) */;
extern int swprintf (wchar_t *, size_t, const wchar_t *, ...)
  /* __attribute__((__format__ (__wprintf__, 3, 4))) */;
#endif

#define _(String) gettext (String)
#define C_(Context, String) pgettext (Context, String)
#define N_(String) gettext_noop (String)
#define NC_(Context, String) gettext_noop (String)

#undef dgettext
#undef dcgettext
#undef dngettext
#undef dcngettext
#undef pgettext_aux
#undef npgettext_aux
#define dgettext dgettext_rfb
#define dcgettext dcgettext_rfb
#define dngettext dngettext_rfb
#define dcngettext dcngettext_rfb
#define pgettext_aux pgettext_rfb
#define npgettext_aux npgettext_rfb

#ifdef __cplusplus
extern "C" {
#endif

const char *dgettext_rfb(const char *domainname, const char *msgid)
                         __attribute__ ((format_arg (2)));
const char *dcgettext_rfb(const char *domainname, const char *msgid,
                          int category)
                          __attribute__ ((format_arg (2)));
const char *dngettext_rfb(const char *domainname, const char *msgid,
                          const char *msgid_plural,
                          unsigned long int n)
                          __attribute__ ((format_arg (2)))
                          __attribute__ ((format_arg (3)));
const char *dcngettext_rfb(const char *domainname, const char *msgid,
                           const char *msgid_plural,
                           unsigned long int n, int category)
                          __attribute__ ((format_arg (2)))
                          __attribute__ ((format_arg (3)));

const char *pgettext_rfb(const char *domain,
                         const char *msg_ctxt_id, const char *msgid,
                         int category)
                         __attribute__ ((format_arg (3)));
const char *npgettext_rfb(const char *domain,
                          const char *msg_ctxt_id, const char *msgid,
                          const char *msgid_plural,
                          unsigned long int n, int category)
                          __attribute__ ((format_arg (3)))
                          __attribute__ ((format_arg (4)));

#ifdef __cplusplus
}
#endif

#endif
