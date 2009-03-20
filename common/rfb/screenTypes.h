/* Copyright 2009 Pierre Ossman for Cendio AB
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
#ifndef __RFB_SCREENTYPES_H__
#define __RFB_SCREENTYPES_H__

namespace rfb {

  // Reasons
  const unsigned int reasonServer = 0;
  const unsigned int reasonClient = 1;
  const unsigned int reasonOtherClient = 2;

  // Result codes
  const unsigned int resultSuccess = 0;
  const unsigned int resultProhibited = 1;
  const unsigned int resultNoResources = 2;
  const unsigned int resultInvalid = 3;

  const int resultUnsolicited = 0xffff; // internal code used for server changes

}
#endif
