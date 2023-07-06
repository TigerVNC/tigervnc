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

#ifndef _I18N_H
#define _I18N_H 1

/* Need to tell gcc that pgettext() doesn't screw up format strings */
#ifdef __GNUC__
static const char *
pgettext_aux (const char *domain,
              const char *msg_ctxt_id, const char *msgid,
              int category) __attribute__ ((format_arg (3)));
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
#define p_(Context, String) pgettext (Context, String)
#define N_(String) gettext_noop (String)

#endif /* _I18N_H */
