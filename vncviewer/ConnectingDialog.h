/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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

// -=- ConnectingDialog.h

// Dialog to indicate to the user that the viewer is attempting to make an
// outgoing connection.

#ifndef __RFB_WIN32_CONNECTING_DLG_H__
#define __RFB_WIN32_CONNECTING_DLG_H__

#include <rfb_win32/Dialog.h>
#include <rfb/Threading.h>
#include <vncviewer/resource.h>

namespace rfb {

  namespace win32 {

    BOOL CALLBACK ConnectingDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	    switch (uMsg) {
	    case WM_INITDIALOG:
		    {
			    SetWindowLong(hwnd, GWL_USERDATA, lParam);
			    return TRUE;
		    }
	    case WM_COMMAND:
		    switch (LOWORD(wParam)) {
		    case IDCANCEL:
          network::Socket* sock = (network::Socket*) GetWindowLong(hwnd, GWL_USERDATA);
          sock->shutdown();
			    EndDialog(hwnd, FALSE);
			    return TRUE;
		    }
		    break;
	    case WM_DESTROY:
		    EndDialog(hwnd, TRUE);
		    return TRUE;
	    }
	    return 0;
    }

    // *** hacky bit - should use async connect so dialog behaves properly
    class ConnectingDialog : public Thread {
    public:
      ConnectingDialog() : Thread("ConnectingDialog") {
        dialog = 0;
        active = true;
        start();
      }
      virtual ~ConnectingDialog() {
        // *** join() required here because otherwise ~Thread calls Thread::join()
        join();
      }
      virtual void run() {
        dialog = CreateDialogParam(GetModuleHandle(0),
          MAKEINTRESOURCE(IDD_CONNECTING_DLG), 0, &ConnectingDlgProc, 0);
        ShowWindow(dialog, SW_SHOW);
        MSG msg;
        while (active && GetMessage(&msg, dialog, 0, 0)) {
          DispatchMessage(&msg);
        }
        DestroyWindow(dialog);
      }
      virtual Thread* join() {
        active = false;
        if (dialog)
          PostMessage(dialog, WM_QUIT, 0, 0);
        return Thread::join();
      }
    protected:
      HWND dialog;
      bool active;
    };

  };

};

#endif