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

// -=- TransferQueue.

#include <rfb/TransferQueue.h>

using namespace rfb;

TransferQueue::TransferQueue()
{
	 m_numEntries = 0;
     m_pEntries = NULL;
}

TransferQueue::~TransferQueue()
{
  free();
}

void
TransferQueue::add(TransferQueue *pTQ)
{
  for (unsigned int i = 0; i < pTQ->getNumEntries(); i++) {
    add(pTQ->getLocPathAt(i), pTQ->getRemPathAt(i),	pTQ->getLocNameAt(i), 
      pTQ->getRemNameAt(i), pTQ->getSizeAt(i), pTQ->getDataAt(i), pTQ->getFlagsAt(i));
  }
}

void
TransferQueue::add(char *pLocPath, char *pRemPath, FileInfo *pFI, unsigned int flags)
{
  char locPath[FT_FILENAME_SIZE];
  char remPath[FT_FILENAME_SIZE];
  strcpy(locPath, pLocPath);
  strcpy(remPath, pRemPath);
  
  for (unsigned int i = 0; i < pFI->getNumEntries(); i++) {
    add(locPath, remPath, pFI->getNameAt(i), pFI->getNameAt(i), 
      pFI->getSizeAt(i), pFI->getDataAt(i), (pFI->getFlagsAt(i) | flags));
  }
}

void 
TransferQueue::add(char *pLocPath, char *pRemPath, char *pLocName, char *pRemName, 
                   unsigned int size, unsigned int data, unsigned int flags)
{
  FILEINFOEX *pTemporary = new FILEINFOEX[m_numEntries + 1];
  if (m_numEntries != 0) 
    memcpy(pTemporary, m_pEntries, m_numEntries * sizeof(FILEINFOEX));
  
  strcpy(pTemporary[m_numEntries].locPath, pLocPath);
  strcpy(pTemporary[m_numEntries].locName, pLocName);
  strcpy(pTemporary[m_numEntries].remPath, pRemPath);
  strcpy(pTemporary[m_numEntries].remName, pRemName);
  
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
TransferQueue::getLocPathAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].locPath;
  }
  return NULL;
}

char *
TransferQueue::getLocNameAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].locName;
  }
  return NULL;
}

char *
TransferQueue::getRemPathAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].remPath;
  }
  return NULL;
}

char *
TransferQueue::getRemNameAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].remName;
  }
  return NULL;
}

char *
TransferQueue::getFullLocPathAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    sprintf(m_szFullLocPath, "%s\\%s", getLocPathAt(number), getLocNameAt(number));
    return m_szFullLocPath;
  }
  return NULL;
}

char *
TransferQueue::getFullRemPathAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    sprintf(m_szFullRemPath, "%s\\%s", getRemPathAt(number), getRemNameAt(number));
    return m_szFullRemPath;
  }
  return NULL;
}

SIZEDATAFLAGSINFO * 
TransferQueue::getSizeDataFlagsAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return &m_pEntries[number].info;
  }
  return NULL;
}

bool 
TransferQueue::setLocPathAt(unsigned int number, char *pName)
{
  if ((number >= 0) && (number < m_numEntries)) {
    strcpy(m_pEntries[number].locPath, pName);
    return true;
  }
  return false;
}

bool 
TransferQueue::setLocNameAt(unsigned int number, char *pName)
{
  if ((number >= 0) && (number < m_numEntries)) {
    strcpy(m_pEntries[number].locName, pName);
    return true;
  }
  return false;
}

bool 
TransferQueue::setRemPathAt(unsigned int number, char *pName)
{
  if ((number >= 0) && (number < m_numEntries)) {
    strcpy(m_pEntries[number].remPath, pName);
    return true;
  }
  return false;
}

bool 
TransferQueue::setRemNameAt(unsigned int number, char *pName)
{
  if ((number >= 0) && (number < m_numEntries)) {
    strcpy(m_pEntries[number].remName, pName);
    return true;
  }
  return false;
}

unsigned int
TransferQueue::getSizeAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].info.size;
  }
  return 0;
}

unsigned int
TransferQueue::getDataAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].info.data;
  }
  return 0;
}

unsigned int 
TransferQueue::getFlagsAt(unsigned int number)
{
  if ((number >= 0) && (number < m_numEntries)) {
    return m_pEntries[number].info.flags;
  }
  return 0;
}

bool 
TransferQueue::setSizeAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.size = value;
    return true;
  }
  return false;
}

bool 
TransferQueue::setDataAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.data = value;
    return true;
  }
  return false;
}

bool 
TransferQueue::setFlagsAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.flags = m_pEntries[number].info.flags | value;
    return true;
  }
  return false;
}

bool 
TransferQueue::clearFlagAt(unsigned int number, unsigned int value)
{
  if ((number >= 0) && (number < m_numEntries)) {
    m_pEntries[number].info.flags = (m_pEntries[number].info.flags & (value ^ 0xFFFFFFFF));
    return true;
  }
  return false;
}

bool 
TransferQueue::setFlagToAll(unsigned int flag)
{
  for (unsigned int i = 0; i < m_numEntries; i++) {
    setFlagsAt(i, flag);
  }
  return true;
}

bool 
TransferQueue::deleteAt(unsigned int number)
{
  if ((number >= m_numEntries) || (number < 0)) return false;
  
  FILEINFOEX *pTemporary = new FILEINFOEX[m_numEntries - 1];
  
  if (number == 0) {
    memcpy(pTemporary, &m_pEntries[1], (m_numEntries - 1) * sizeof(FILEINFOEX));
  } else {
    memcpy(pTemporary, m_pEntries, number * sizeof(FILEINFOEX));
    if (number != (m_numEntries - 1)) 
      memcpy(&pTemporary[number], &m_pEntries[number + 1], (m_numEntries - number - 1) * sizeof(FILEINFOEX));
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
TransferQueue::getNumEntries()
{
  return m_numEntries;
}

void 
TransferQueue::free()
{
  if (m_pEntries != NULL) {
    delete [] m_pEntries;
    m_pEntries = NULL;
  }
  m_numEntries = 0;
}
