// ControlPanel.h: interface for the ControlPanel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef AFX_CONTROLPANEL_H__
#define AFX_CONTROLPANEL_H__


#pragma once


#include <list>

#include <winvnc/VNCServerWin32.h>
#include <winvnc/resource.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/ListViewControl.h>
#include <rfb_win32/Win32Util.h>

namespace winvnc {
  
  class ControlPanel : rfb::win32::Dialog, rfb::win32::ListViewControl {
  public:
    ControlPanel(VNCServerWin32 * server, HWND hSTIcon) : Dialog(GetModuleHandle(0)), ListViewControl(){
      m_server = server;
      m_hSTIcon = hSTIcon;
    };
    virtual bool showDialog();
    virtual void initDialog();
    virtual bool onCommand(int cmd);
    void UpdateListView();
    HWND GetHandle() {return handle;};
    ~ControlPanel();
  protected: 
    virtual BOOL dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void getSelectedConn(std::list<network::Socket*>* selsockets);
    VNCServerWin32 * m_server;
    std::list<network::Socket*> sockets;
    HWND m_hSTIcon;
  };
};

#endif  