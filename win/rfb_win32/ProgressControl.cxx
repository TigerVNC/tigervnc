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

// -=- ProgressControl.cxx

#include <rfb_win32/ProgressControl.h>

using namespace rfb;
using namespace rfb::win32;

#define MAX_RANGE 0xFFFF

ProgressControl::ProgressControl(HWND hwndProgress)
{
  m_hwndProgress = hwndProgress;
  
  m_dw64MaxValue = 0;
  m_dw64CurrentValue = 0;
}

ProgressControl::~ProgressControl()
{
}

bool
ProgressControl::init(DWORD64 maxValue, DWORD64 position)
{
  if (m_dw64CurrentValue > m_dw64MaxValue) return false;
  
  m_dw64CurrentValue = position;
  m_dw64MaxValue = maxValue;
  
  if (!SendMessage(m_hwndProgress, PBM_SETRANGE, (WPARAM) 0, MAKELPARAM(0, MAX_RANGE))) 
    return false;
  
  return true;
}

bool
ProgressControl::clear()
{
  m_dw64CurrentValue = 0;
  return show();
}

bool
ProgressControl::increase(DWORD64 value)
{
  if ((m_dw64MaxValue - m_dw64CurrentValue) > value) {
    m_dw64CurrentValue += value;
  } else {
    m_dw64CurrentValue = m_dw64MaxValue;
  }
  return show();
}

bool
ProgressControl::show()
{
  DWORD curPos;
  if (m_dw64MaxValue != 0) {
    curPos = (DWORD) ((m_dw64CurrentValue * MAX_RANGE) / m_dw64MaxValue);
  } else {
    curPos = 0;
  }
  
  if (!SendMessage(m_hwndProgress, PBM_SETPOS, (WPARAM) curPos, (LPARAM) 0))
    return false;
  
  return true;
}

int 
ProgressControl::getCurrentPercent()
{
  if (m_dw64MaxValue == 0) return 0;

  return ((int) ((m_dw64CurrentValue * 100) / m_dw64MaxValue));
}
