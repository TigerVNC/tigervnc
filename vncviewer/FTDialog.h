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

// -=- FTDialog.h

#ifndef __RFB_WIN32_FTDIALOG_H__
#define __RFB_WIN32_FTDIALOG_H__

#include <windows.h>
#include <commctrl.h>

#include <rfb/FileInfo.h>
#include <rfb_win32/Dialog.h>
#include <vncviewer/FileTransfer.h>
#include <vncviewer/FTListView.h>
#include <vncviewer/FTProgress.h>
#include <vncviewer/FTBrowseDlg.h>
#include <vncviewer/resource.h>

namespace rfb {
  namespace win32 {
    class FileTransfer;
    class FTBrowseDlg;

    class FTDialog
    {
    public:
      FTDialog(HINSTANCE hInst, FileTransfer *pFT);
      ~FTDialog();
      
      bool createFTDialog(HWND hwndParent);
      bool closeFTDialog();
      void destroyFTDialog();

      void processDlgMsgs();

      void cancelTransfer(bool bResult);
      void afterCancelTransfer();
      
      static BOOL CALLBACK FTDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
      
      void addRemoteLVItems(FileInfo *pFI);
      void reqFolderUnavailable();

      void onEndBrowseDlg(bool bResult);
      void getBrowseItems(char *pPath);
      void addBrowseItems(FileInfo *pFI);

      void setStatusText(LPCSTR format,...);

      HWND getWndHandle() { return m_hwndFTDialog; }

      void postCheckDeleteQueueMsg();
      void postCheckTransferQueueMsg();
      void postUploadFilePortionMsg();
      void postDownloadFilePortionMsg();

      char *getLocalPath() { return m_szLocalPath; }
      char *getRemotePath() { return m_szRemotePath; }

      FTProgress *m_pProgress;
      FileTransfer *m_pFileTransfer;
      
    private:
      HWND m_hwndFTDialog;
      HWND m_hwndLocalPath;
      HWND m_hwndRemotePath;
      HINSTANCE m_hInstance;
      
      void showLocalLVItems();
      void showRemoteLVItems();

      void onClose();

      void onLocalItemActivate(LPNMITEMACTIVATE lpnmia);
      void onRemoteItemActivate(LPNMITEMACTIVATE lpnmia);

      void onLocalBrowse();
      void onRemoteBrowse();

      void onLocalReload();
      void onRemoteReload();

      void onLocalRButton();
      void onRemoteRButton();

      bool getCreateFolderName();
      void onLocalCreateFolder();
      void onRemoteCreateFolder();

      void showFTMenu(bool copyBtnState, bool createFldBtnState, 
                      bool renameBtnState, bool deleteBtnState, 
                      bool cancelBtnState);
      void onFTMenuCommand(int command);

      void onUpload();
      void onDownload();

      void onLocalRename();
      void onRemoteRename();

      void onLocalDelete();
      void onRemoteDelete();

      void onFTCancel();

      void setIcon(int dest, int idIcon);
      bool initFTDialog();
      bool initFTWndMsgs();
      
      void onLocalOneUpFolder();
      void onRemoteOneUpFolder();
      int makeOneUpFolder(char *pPath);

      void refreshBtnState();
      void setButtonsState();
      
      bool m_bDlgShown;
      bool m_bLocalBrowsing;
      bool m_bCloseDlgAfterCancel;

      UINT m_msgCheckDeleteQueue;
      UINT m_msgCheckTransferQueue;
      UINT m_msgUploadFilePortion;
      UINT m_msgDownloadFilePortion;

      DWORD m_dwNumStatusStrings;

      FTListView *m_pLocalLV;
      FTListView *m_pRemoteLV;

      FTBrowseDlg *m_pBrowseDlg;

      int m_FTMenuSource;

      char m_szLocalPath[FT_FILENAME_SIZE];
      char m_szRemotePath[FT_FILENAME_SIZE];
      char m_szLocalPathTmp[FT_FILENAME_SIZE];
      char m_szRemotePathTmp[FT_FILENAME_SIZE];
      char m_szCreateFolderName[FT_FILENAME_SIZE];

      static const char szCheckDeleteQueueText[];
      static const char szCheckTransferQueueText[];
      static const char szUploadFilePortionText[];

      typedef struct tagFTBUTTONSSTATE
      {
        bool uploadBtn;
        bool downloadBtn;
        bool createLocalFldBtn;
        bool createRemoteFldBtn;
        bool renameLocalBtn;
        bool renameRemoteBtn;
        bool deleteLocalBtn;
        bool deleteRemoteBtn;
        bool cancelBtn;
      } FTBUTTONSSTATE;

      FTBUTTONSSTATE m_BtnState;

    public:
      class CancelingDlg
      {
      public:
        CancelingDlg(FTDialog *pFTDlg);
        ~CancelingDlg();

        static BOOL CALLBACK cancelingDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        bool create();
        bool destroy();

      private:
        FTDialog *m_pFTDlg;
        HWND m_hwndDlg;

        bool close(bool bResult);
      };

    private:
        CancelingDlg *m_pCancelingDlg;

    public:
      class CreateFolderDlg : public Dialog
      {
      public:
        CreateFolderDlg(FTDialog *pFTDlg);
        ~CreateFolderDlg();

        bool onOk();
        bool create();
        char *getFolderName() { return m_szFolderName; }

      protected:
        FTDialog *m_pFTDlg;
        char m_szFolderName[FT_FILENAME_SIZE];
      };

      private:
        CreateFolderDlg *m_pCreateFolderDlg;

    public:
      class RenameDlg : public CreateFolderDlg
      {
      public:
        RenameDlg(FTDialog *pFTDlg);
        ~RenameDlg();

        bool create(char *pFilename, bool bFolder);
        void initDialog();

      private:
        bool m_bFolder;
        char m_szFilename[FT_FILENAME_SIZE];
      };

    private:
        RenameDlg *m_pRenameDlg;
    };
  }
}

#endif // __RFB_WIN32_FTDIALOG_H__
