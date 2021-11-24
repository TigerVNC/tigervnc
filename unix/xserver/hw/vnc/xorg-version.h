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

#define XORG_AT_LEAST(major, minor, patch) \
    (XORG_VERSION_CURRENT >= ((major * 10000000) + (minor * 100000) + (patch * 1000)))
#define XORG_OLDER_THAN(major, minor, patch) \
    (XORG_VERSION_CURRENT < ((major * 10000000) + (minor * 100000) + (patch * 1000)))

#if XORG_OLDER_THAN(1, 16, 0)
#error "X.Org older than 1.16 is not supported"
#endif

#if XORG_AT_LEAST(1, 22, 0)
#error "X.Org newer than 1.21 is not supported"
#endif

#endif
