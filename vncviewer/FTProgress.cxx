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
}

FTProgress::~FTProgress()
{
  destroyProgressBarObjects();
}

bool
FTProgress::initialize(DWORD64 totalMaxValue, DWORD maxValue)
{
  m_bInitialized = false;

  m_hwndSinglePercent = GetDlgItem(m_hwndParent, IDC_FTSINGLEPERCENT);
  m_hwndGeneralPercent = GetDlgItem(m_hwndParent, IDC_FTGENERALPERCENT);

  if ((m_hwndSinglePercent == NULL) || (m_hwndGeneralPercent == NULL)) return false;
 
  if (!createProgressBarObjects()) return false;

  if (!initProgressControls(totalMaxValue, maxValue)) return false;

  setProgressText();

  m_bInitialized = true;
  return true;
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
FTProgress::clearSingle()
{
  if (!m_bInitialized) return;

  m_pSingleProgress->clear();

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
  }

  if (m_pGeneralProgress != NULL) {
    delete m_pGeneralProgress;
  }

  return true;
}

bool
FTProgress::initProgressControls(DWORD64 totalMaxValue, DWORD maxValue)
{
  bool bResult = true;

  if ((m_pSingleProgress != NULL) && (m_pGeneralProgress != NULL)) {
    if (!m_pSingleProgress->init(totalMaxValue, 0)) return false;
    if (!m_pGeneralProgress->init(maxValue, 0)) return false;
  } else {
    return false;
  }

  setProgressText();
  return true;
}

void
FTProgress::setProgressText()
{
  	char buf[16];

    int percent = m_pSingleProgress->getCurrentPercent();
	sprintf(buf, "%d%%", percent);
	SetWindowText(m_hwndSinglePercent, buf);

    percent = m_pGeneralProgress->getCurrentPercent();
	sprintf(buf, "%d%%", percent);
	SetWindowText(m_hwndGeneralPercent, buf);
}