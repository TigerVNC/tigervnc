/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <rfb/encodings.h>
#include <rfb/util.h>

int rfb::encodingNum(const char* name)
{
  if (strcasecmp(name, "raw") == 0)      return encodingRaw;
  if (strcasecmp(name, "copyRect") == 0) return encodingCopyRect;
  if (strcasecmp(name, "RRE") == 0)      return encodingRRE;
  if (strcasecmp(name, "CoRRE") == 0)    return encodingCoRRE;
  if (strcasecmp(name, "hextile") == 0)  return encodingHextile;
  if (strcasecmp(name, "ZRLE") == 0)     return encodingZRLE;
  if (strcasecmp(name, "Tight") == 0)    return encodingTight;
#ifdef HAVE_H264
  if (strcasecmp(name, "H.264") == 0)    return encodingH264;
#endif
  return -1;
}

const char* rfb::encodingName(int num)
{
  switch (num) {
  case encodingRaw:      return "raw";
  case encodingCopyRect: return "copyRect";
  case encodingRRE:      return "RRE";
  case encodingCoRRE:    return "CoRRE";
  case encodingHextile:  return "hextile";
  case encodingZRLE:     return "ZRLE";
  case encodingTight:    return "Tight";
#ifdef HAVE_H264
  case encodingH264:     return "H.264";
#endif
  default:               return "[unknown encoding]";
  }
}
