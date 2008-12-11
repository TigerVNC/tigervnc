/* Copyright (C) 2008 TightVNC Team.  All Rights Reserved.
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

#ifndef OS_NET_H
#define OS_NET_H

#ifdef HAVE_COMMON_CONFIG_H
#include <common-config.h>
#endif

#ifdef WIN32
#include <common-config.win.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

/* IPv6 support on server side - we have to have all those functions */
#if defined(HAVE_INET_NTOP)
#define HAVE_IPV6
#endif

/* IPv4-only stub implementation */
#ifndef HAVE_INET_NTOP
const char *tight_inet_ntop(int af, const void *src,
			    char *dst, socklen_t size);
#define inet_ntop tight_inet_ntop
#endif

#ifdef __cplusplus
};
#endif

#endif /* OS_NET_H */
