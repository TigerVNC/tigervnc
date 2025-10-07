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

#include <assert.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "../../w0vncserver.h"
#include "Display.h"
#include "Object.h"

using namespace wayland;

Object::Object(Display* display, const char* interfaceName,
                 const wl_interface* interface)
  : boundObject(nullptr)
{
  wl_registry* registry;
  ObjectInfo* objectInfo;

  assert(display->interfaceAvailable(interfaceName));

  objectInfo = display->getObjectInfo(interfaceName);
  registry = display->getRegistry();

  boundObject = (wl_proxy*)wl_registry_bind(registry, objectInfo->name,
                                            interface,
                                            objectInfo->version);

  if (!boundObject) {
    fatal_error("Failed to bind to %s ", interfaceName);
    return;
  }
}
