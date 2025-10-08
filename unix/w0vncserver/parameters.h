/* Copyright 2025 Adam Halim for Cendio AB
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

#ifndef __W0VNCSERVER_PARAMETERS_H__
#define __W0VNCSERVER_PARAMETERS_H__

#include <core/Configuration.h>

extern core::IntParameter rfbport;
extern core::StringParameter rfbunixpath;
extern core::IntParameter rfbunixmode;
extern core::BoolParameter localhostOnly;
extern core::StringParameter desktopName;
extern core::StringParameter interface;

#endif // __W0VNCSERVER_PARAMETERS_H__
