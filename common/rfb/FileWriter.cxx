/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
 *
 * Developed by Dennis Syrovatsky.
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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- FileWriter.cxx

#include <rfb/FileWriter.h>

using namespace rfb;

FileWriter::FileWriter()
{
  strcpy(m_szMode, "wb");
}

bool 
FileWriter::write(const void *pBuf, unsigned int count, unsigned int *pBytesWritten)
{
  if (!isCreated()) return false;

  unsigned int bytesWritten = fwrite(pBuf, 1, count, m_pFile);

  if (ferror(m_pFile)) return false;

  *pBytesWritten = bytesWritten;
  return true;
}

bool 
FileWriter::setTime(unsigned int modTime)
{
  return false;
}
