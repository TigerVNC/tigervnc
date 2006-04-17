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

// -=- FTBrpwseDlg.cxx

#include <vncviewer/FTBrowseDlg.h>

using namespace rfb;
using namespace rfb::win32;

FTBrowseDlg::FTBrowseDlg(FTDialog *pFTDlg)
{
  m_pFTDlg = pFTDlg;
  m_hwndDlg = NULL;
  m_hwndTree = NULL;
  m_hParentItem = NULL;
}

FTBrowseDlg::~FTBrowseDlg()
{
  destroy();
}

bool
FTBrowseDlg::create()
{
  m_hwndDlg = CreateDialogParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_FTBROWSE), 
                                m_pFTDlg->getWndHandle(), (DLGPROC) FTBrowseDlgProc, 
                                (LONG) this);

  if (m_hwndDlg == NULL) return false;

  m_hwndTree = GetDlgItem(m_hwndDlg, IDC_FTBROWSETREE);

  ShowWindow(m_hwndDlg, SW_SHOW);
  UpdateWindow(m_hwndDlg);

  return true;
}

void
FTBrowseDlg::destroy()
{
  EndDialog(m_hwndDlg, 0);
}

void
FTBrowseDlg::addItems(FileInfo *pFI)
{
  TVITEM tvi;
  TVINSERTSTRUCT tvins;

  if (pFI->getNumEntries() <= 0) return;

  for (unsigned int i = 0; i < pFI->getNumEntries(); i++)
  {
    tvi.mask = TVIF_TEXT;
    tvi.pszText = pFI->getNameAt(i);;
    tvins.hParent = m_hParentItem;
    tvins.item = tvi;
    tvins.hParent = TreeView_InsertItem(m_hwndTree, &tvins);
    TreeView_InsertItem(m_hwndTree, &tvins);
  }
}

char *
FTBrowseDlg::getTVPath(HTREEITEM hTItem)
{
  char path[FT_FILENAME_SIZE];
  char szText[FT_FILENAME_SIZE];

  TVITEM tvi;
  path[0] = '\0';

  do {
    tvi.mask = TVIF_TEXT | TVIF_HANDLE;
    tvi.hItem = hTItem;
    tvi.pszText = szText;
    tvi.cchTextMax = FT_FILENAME_SIZE;
    TreeView_GetItem(m_hwndTree, &tvi);
    sprintf(path, "%s\\%s", path, tvi.pszText);
    hTItem = TreeView_GetParent(m_hwndTree, hTItem);
  } while(hTItem != NULL);

  return pathInvert(path);
}

char *
FTBrowseDlg::pathInvert(char *pPath)
{
  int len = strlen(pPath);
  m_szPath[0] = '\0';
  char *pos = NULL;
  
  while ((pos = strrchr(pPath, '\\')) != NULL) {
    if (strlen(m_szPath) == 0) {
      strcpy(m_szPath, (pos + 1));
    } else {
      sprintf(m_szPath, "%s\\%s", m_szPath, (pos + 1));
    }
    *pos = '\0';
  }

  m_szPath[len] = '\0';
  return m_szPath;
}

char *
FTBrowseDlg::getPath()
{
  GetDlgItemText(m_hwndDlg, IDC_FTBROWSEPATH, m_szPath, FT_FILENAME_SIZE);
  return m_szPath;
}

void
FTBrowseDlg::deleteChildItems()
{
  while (TreeView_GetChild(m_hwndTree, m_hParentItem) != NULL) {
    TreeView_DeleteItem(m_hwndTree, TreeView_GetChild(m_hwndTree, m_hParentItem));
  }
}

BOOL CALLBACK 
FTBrowseDlg::FTBrowseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  FTBrowseDlg *_this = (FTBrowseDlg *) GetWindowLong(hwnd, GWL_USERDATA);
  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      return FALSE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDOK:
        _this->m_pFTDlg->onEndBrowseDlg(true);
        return FALSE;
      case IDCANCEL:
        _this->m_pFTDlg->onEndBrowseDlg(false);
        return FALSE;
      }
    }
    break;
  case WM_NOTIFY:
    switch (LOWORD(wParam))
    {
    case IDC_FTBROWSETREE:
      switch (((LPNMHDR) lParam)->code)
      {
      case TVN_SELCHANGED:
        SetDlgItemText(hwnd, IDC_FTBROWSEPATH, _this->getTVPath(((NMTREEVIEW *) lParam)->itemNew.hItem));
        return FALSE;
//      case TVN_ITEMEXPANDING:
      case TVN_ITEMEXPANDED:
        {
          NMTREEVIEW *nmCode = (NMTREEVIEW *) lParam;
          if (nmCode->action == 2) {
            _this->m_hParentItem = nmCode->itemNew.hItem;
            _this->deleteChildItems();
            _this->m_pFTDlg->getBrowseItems(_this->getTVPath(_this->m_hParentItem));
          }
        }
        return FALSE;
      }
    break;
    case WM_CLOSE:
      _this->m_pFTDlg->onEndBrowseDlg(false);
      return FALSE;
    }
  }
    return 0;
}
