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

#ifndef COMMON_RFB_ACCESSRIGHTS_H_
#define COMMON_RFB_ACCESSRIGHTS_H_

#include <stdint.h>

namespace rfb
{

  typedef uint16_t AccessRights;
  extern const AccessRights AccessNone;           // No rights at all
  extern const AccessRights AccessView;           // View display contents
  extern const AccessRights AccessKeyEvents;      // Send key events
  extern const AccessRights AccessPtrEvents;      // Send pointer events
  extern const AccessRights AccessCutText;        // Send/receive clipboard events
  extern const AccessRights AccessSetDesktopSize; // Change desktop size
  extern const AccessRights AccessNonShared;      // Exclusive access to the server
  extern const AccessRights AccessDefault;        // The default rights, INCLUDING FUTURE ONES
  extern const AccessRights AccessNoQuery;        // Connect without local user accepting
  extern const AccessRights AccessFull;           // All of the available AND FUTURE rights

} /* namespace rfb */

#endif /* COMMON_RFB_ACCESSRIGHTS_H_ */
