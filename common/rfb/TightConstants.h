/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
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
#ifndef __RFB_TIGHTCONSTANTS_H__
#define __RFB_TIGHTCONSTANTS_H__
namespace rfb {
  // Compression control 
  const unsigned int tightExplicitFilter = 0x04;
  const unsigned int tightFill = 0x08;
  const unsigned int tightJpeg = 0x09;
  const unsigned int tightMaxSubencoding = 0x09;

  // Filters to improve compression efficiency
  const unsigned int tightFilterCopy = 0x00;
  const unsigned int tightFilterPalette = 0x01;
  const unsigned int tightFilterGradient = 0x02;
}
#endif
