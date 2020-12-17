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

#if XORG_VERSION_CURRENT < ((1 * 10000000) + (15 * 100000) + (99 * 1000))
#error "X.Org older than 1.16 is not supported"
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (16 * 100000) + (99 * 1000))
#define XORG 116
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (17 * 100000) + (99 * 1000))
#define XORG 117
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (18 * 100000) + (99 * 1000))
#define XORG 118
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (19 * 100000) + (99 * 1000))
#define XORG 119
#elif XORG_VERSION_CURRENT < ((1 * 10000000) + (20 * 100000) + (99 * 1000))
#define XORG 120
#else
#error "X.Org newer than 1.20 is not supported"
#endif

#endif
