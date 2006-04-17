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

#ifndef __RFB_TRANSFERQUEUE_H__
#define __RFB_TRANSFERQUEUE_H__

#include <stdlib.h>

#include <rfb/FileInfo.h>
#include <rfb/fttypes.h>

namespace rfb {
  class TransferQueue  
  {
  public:
    TransferQueue();
    ~TransferQueue();

    void add(TransferQueue *pTQ);
    void add(char *pLocPath, char*pRemPath, FileInfo *pFI, unsigned int flags);
    void add(char *pLocPath, char *pRemPath, char *pLocName, char *pRemName,
      unsigned int size, unsigned int data, unsigned int flags);
    
    char *getLocPathAt(unsigned int number);
    char *getLocNameAt(unsigned int number);
    char *getRemPathAt(unsigned int number);
    char *getRemNameAt(unsigned int number);
    
    char *getFullLocPathAt(unsigned int number);
    char *getFullRemPathAt(unsigned int number);
    
    bool setLocPathAt(unsigned int number, char *pName);
    bool setLocNameAt(unsigned int number, char *pName);
    bool setRemPathAt(unsigned int number, char *pName);
    bool setRemNameAt(unsigned int number, char *pName);
    
    unsigned int getSizeAt(unsigned int number);
    unsigned int getDataAt(unsigned int number);
    unsigned int getFlagsAt(unsigned int number);
    
    SIZEDATAFLAGSINFO * getSizeDataFlagsAt(unsigned int number);
    
    
    bool setSizeAt(unsigned int number, unsigned int value);
    bool setDataAt(unsigned int number, unsigned int value);
    bool setFlagsAt(unsigned int number, unsigned int value);
    bool clearFlagAt(unsigned int number, unsigned int value);
    bool setFlagToAll(unsigned int flag);
    
    bool deleteAt(unsigned int number);
    
    unsigned int getNumEntries();
    
    void free();
    
  private:
    FILEINFOEX *m_pEntries;
    unsigned int m_numEntries;
    
    char m_szFullLocPath[FT_FILENAME_SIZE];
    char m_szFullRemPath[FT_FILENAME_SIZE];
  };
}

#endif // __RFB_TRANSFERQUEUE_H__
