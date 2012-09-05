/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009 Red Hat, Inc.
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

#ifndef XORG_VERSION_H
#define XORG_VERSION_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if XORG_VERSION_CURRENT < ((1 * 10000000) + (5 * 100000) + (99 * 1000))
#define XORG 15
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (6 * 100000) + (99 * 1000))
#define XORG 16
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (7 * 100000) + (99 * 1000))
#define XORG 17
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (8 * 100000) + (99 * 1000))
#define XORG 18
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (9 * 100000) + (99 * 1000))
#define XORG 19
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (10 * 100000) + (99 * 1000))
#define XORG 110
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (11 * 100000) + (99 * 1000))
#define XORG 111
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (12 * 100000) + (99 * 1000))
#define XORG 112
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (13 * 100000) + (99 * 1000))
#define XORG 113
#else
#error "X.Org newer than 1.13 is not supported"
#endif

#endif
