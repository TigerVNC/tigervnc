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

#include <rfb/FileInfo.h>
#include <rfb/util.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

using namespace rfb;

// FIXME: Under Unix, file names are case-sensitive.

int 
CompareFileInfo(const void *F, const void *S)
{
  FILEINFO *pF = (FILEINFO *) F;
  FILEINFO *pS = (FILEINFO *) S;
  if (pF->info.flags == pS->info.flags) {
    return strcasecmp(pF->name, pS->name);
  } else {
	if (pF->info.flags == FT_ATTR_DIR) return -1;
	if (pS->info.flags == FT_ATTR_DIR)
      return 1;
	else
      return strcasecmp(pF->name, pS->name);
  }
  
  return 0;
}

FileInfo::FileInfo()
{
  m_numEntries = 0;
  m_pEntries = NULL;
}

FileInfo::~FileInfo()
{
  free();
}

void 
FileInfo::add(FileInfo *pFI)
{
  m_numEntries = pFI->getNumEntries();
  FILEINFO *pTemporary = new FILEINFO[m_numEntries];
  memcpy(pTemporary, pFI->getNameAt(0), m_numEntries * sizeof(FILEINFO));
  
  m_pEntries = pTemporary;
  pTemporary = NULL;
}

void 
FileInfo::add(FILEINFO *pFIStruct)
{
  add(pFIStruct->name, pFIStruct->info.size, pFIStruct->info.data, pFIStruct->info.flags);
}

void 
FileInfo::add(char *pName, unsigned int size, unsigned int data, unsigned int flags)
{
  FILEINFO *pTemporary = new FILEINFO[m_numEntries + 1];
  if (m_numEntries != 0) 
    memcpy(pTemporary, m_pEntries, m_numEntries * sizeof(FILEINFO));
  strcpy(pTemporary[m_numEntries].name, pName);
  pTemporary[m_numEntries].info.size = size;
  pTemporary[m_numEntries].info.data = data;
  pTemporary[m_numEntries].info.flags = flags;
  if (m_pEntries != NULL) {
    delete [] m_pEntries;
    m_pEntries = NULL;
  }
  m_pEntries = pTemporary;
  pTemporary = NULL;
  m_numEntries++;
}

char *
FileInfo::getNameAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].name;
  }
  return NULL;
}

bool 
FileInfo::setNameAt(unsigned int number, char *pName)
{
  if ((number >= 0) && (number < m_numEntries)) {
    strcpy(m_pEntries[number].name, pName);
    return true;
  }
  return false;
}

unsigned int
FileInfo::getSizeAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].info.size;
  }
  return 0;
}

unsigned int
FileInfo::getDataAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].info.data;
  }
  return 0;
}

unsigned int
FileInfo::getFlagsAt(unsigned int number)
{
	if ((number >= 0) && (number < m_numEntries)) {
		return m_pEntries[number].info.flags;
	}
	return 0;
}

FILEINFO * 
FileInfo::getFullDataAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return &m_pEntries[number];
  }
  return NULL;
}
	
bool 
FileInfo::setSizeAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.size = value;
    return true;
  }
  return false;
}

bool 
FileInfo::setDataAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.data = value;
    return true;
  }
  return false;
}

bool 
FileInfo::setFlagsAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.flags = value;
    return true;
  }
  return false;
}

bool 
FileInfo::deleteAt(unsigned int number)
{
  if ((number >= m_numEntries) || (number < 0)) return false;
  
  FILEINFO *pTemporary = new FILEINFO[m_numEntries - 1];
  
  if (number == 0) {
    memcpy(pTemporary, &m_pEntries[1], (m_numEntries - 1) * sizeof(FILEINFO));
  } else {
    memcpy(pTemporary, m_pEntries, number * sizeof(FILEINFO));
    if (number != (m_numEntries - 1)) 
      memcpy(&pTemporary[number], &m_pEntries[number + 1], (m_numEntries - number - 1) * sizeof(FILEINFO));
  }
  
  if (m_pEntries != NULL) {
    delete [] m_pEntries;
    m_pEntries = NULL;
  }
  m_pEntries = pTemporary;
  pTemporary = NULL;
  m_numEntries--;
  return true;
}

unsigned int 
FileInfo::getNumEntries()
{
  return m_numEntries;
}

void 
FileInfo::sort()
{
  qsort(m_pEntries, m_numEntries, sizeof(FILEINFO), CompareFileInfo);
}

void 
FileInfo::free()
{
  if (m_pEntries != NULL) {
    delete [] m_pEntries;
    m_pEntries = NULL;
  }
  m_numEntries = 0;
}

unsigned int
FileInfo::getFilenamesSize()
{
  if (getNumEntries() == 0) return 0;

  unsigned int filenamesSize = 0;

  for (unsigned int i = 0; i < getNumEntries(); i++) {
    filenamesSize += strlen(getNameAt(i));
  }

  return filenamesSize;
}
