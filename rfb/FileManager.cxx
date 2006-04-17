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

// -=- FileManager.cxx

#include <rfb/FileManager.h>

using namespace rfb;

FileManager::FileManager()
{
  m_pFile = NULL;
}

FileManager::~FileManager()
{
  close();
}

bool 
FileManager::create(char *pFilename)
{
  if (m_pFile != NULL) return false;
  
  strcpy(m_szFilename, pFilename);

  m_pFile = fopen(m_szFilename, m_szMode);

  if (m_pFile == NULL) {
    return false;
  } else {
    return true;
  }
}

bool 
FileManager::close()
{
  if (m_pFile == NULL) return false;
  
  int result = fclose(m_pFile);
  
  if (result != 0) {
    return false;
  } else {
    m_pFile = NULL;
    return true;
  }
}

bool 
FileManager::isCreated()
{
  if (m_pFile != NULL) return true; else return false;
}

char *
FileManager::getFilename()
{
  return m_szFilename;
}
