/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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

// -=- FileReader.cxx

#include "FileReader.h"

using namespace rfb;

FileReader::FileReader(char *pFilename)
{
  strcpy(m_szFilename, pFilename);
  strcpy(m_szMode, "r");
}

bool 
FileReader::read(void *pBuf, unsigned int count, unsigned int *pBytesRead)
{
  if (m_pFile == NULL) return false;

  unsigned int bytesRead = fread(pBuf, 1, count, m_pFile);

  if (ferror(m_pFile)) return false;

  if (feof(m_pFile)) {
    *pBytesRead = 0;
  } else {
    *pBytesRead = bytesRead;
  }

  return true;
}
