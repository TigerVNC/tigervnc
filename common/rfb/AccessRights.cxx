/* Copyright 2024 TigerVNC Team
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

#include "AccessRights.h"

namespace rfb
{

  // AccessRights values
  const AccessRights AccessNone           = 0x0000;
  const AccessRights AccessView           = 0x0001;
  const AccessRights AccessKeyEvents      = 0x0002;
  const AccessRights AccessPtrEvents      = 0x0004;
  const AccessRights AccessCutText        = 0x0008;
  const AccessRights AccessSetDesktopSize = 0x0010;
  const AccessRights AccessNonShared      = 0x0020;
  const AccessRights AccessDefault        = 0x03ff;
  const AccessRights AccessNoQuery        = 0x0400;
  const AccessRights AccessFull           = 0xffff;

} /* namespace rfb */
