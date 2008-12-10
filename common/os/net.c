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

#ifdef HAVE_COMMON_CONFIG_H
#include <common-config.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <os/net.h>


#ifndef HAVE_INET_NTOP
const char *tight_inet_ntop(int af, const void *src, char *dst,
			    socklen_t size) {
	char *tempstr;

	/* Catch bugs - we should not use IPv6 if we don't have inet_ntop */
	if (af != AF_INET)
		abort();

	/* inet_ntoa never fails */
	tempstr = inet_ntoa(*(struct in_addr *)(src));
	memcpy(dst, tempstr, strlen(tempstr) + 1);

	return dst;
}
#endif
