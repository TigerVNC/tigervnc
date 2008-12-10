/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
 *
 * Developed by Dennis Syrovatsky
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

// -=- FTProgress.cxx

#include <vncviewer/FTProgress.h>

using namespace rfb;
using namespace rfb::win32;

FTProgress::FTProgress(HWND hwndParent)
{
  m_bInitialized = false;
  m_hwndParent = hwndParent;

  m_pSingleProgress = NULL;
  m_pGeneralProgress = NULL;

  m_hwndSinglePercent = GetDlgItem(m_hwndParent, IDC_FTSINGLEPERCENT);
  m_hwndGeneralPercent = GetDlgItem(m_hwndParent, IDC_FTGENERALPERCENT);

  m_bInitialized = createProgressBarObjects();
}

FTProgress::~FTProgress()
{
  destroyProgressBarObjects();
}

void
FTProgress::increase(DWORD value)
{
  if (!m_bInitialized) return;

  m_pSingleProgress->increase(value);
  m_pGeneralProgress->increase(value);

  setProgressText();
}

void
FTProgress::clearAndInitGeneral(DWORD64 dw64MaxValue, DWORD64 dw64Position)
{
  if (!m_bInitialized) return;

  m_pGeneralProgress->clear();
  m_pGeneralProgress->init(dw64MaxValue, dw64Position);

  setProgressText();
}

void
FTProgress::clearAndInitSingle(DWORD dwMaxValue, DWORD dwPosition)
{
  if (!m_bInitialized) return;

  m_pSingleProgress->clear();
  m_pSingleProgress->init(dwMaxValue, dwPosition);

  setProgressText();
}

void
FTProgress::clearAll()
{
  if (!m_bInitialized) return;

  m_pSingleProgress->clear();
  m_pGeneralProgress->clear();

  setProgressText();
}

bool 
FTProgress::createProgressBarObjects()
{
  if ((m_pSingleProgress != NULL) || (m_pGeneralProgress != NULL)) {
    return false;
  } else {
    HWND hwndSingleProgr = GetDlgItem(m_hwndParent, IDC_FTSINGLEPROGRESS);
    HWND hwndGeneralProgr = GetDlgItem(m_hwndParent, IDC_FTGENERALPROGRESS);
    
    m_pSingleProgress = new ProgressControl(hwndSingleProgr);
    if (m_pSingleProgress == NULL) return false;
    
    m_pGeneralProgress = new ProgressControl(hwndGeneralProgr);
    if (m_pGeneralProgress == NULL) {
      delete m_pSingleProgress;
      m_pSingleProgress = NULL;
      return false;
    }
  }
  return true;
}

bool 
FTProgress::destroyProgressBarObjects()
{
  clearAll();

  if (m_pSingleProgress != NULL) {
    delete m_pSingleProgress;
    m_pSingleProgress = NULL;
  }

  if (m_pGeneralProgress != NULL) {
    delete m_pGeneralProgress;
    m_pGeneralProgress = NULL;
  }

  return true;
}

void
FTProgress::setProgressText()
{
  char buf[16] = {0};
  char buf2[16] = {0};
  
  int percent = m_pSingleProgress->getCurrentPercent();
  sprintf(buf, "%d%%", percent);
  GetWindowText(m_hwndSinglePercent, buf2, 16);
  if (strcmp(buf, buf2) != 0)
    SetWindowText(m_hwndSinglePercent, buf);
  
  percent = m_pGeneralProgress->getCurrentPercent();
  sprintf(buf, "%d%%", percent);
  GetWindowText(m_hwndGeneralPercent, buf2, 16);
  if (strcmp(buf, buf2) != 0)
    SetWindowText(m_hwndGeneralPercent, buf);
}
