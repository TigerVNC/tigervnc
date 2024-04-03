// ControlPanel.h: interface for the ControlPanel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef AFX_CONTROLPANEL_H__
#define AFX_CONTROLPANEL_H__


#pragma once


#include <list>
#include <winvnc/resource.h>
#include <winvnc/ListConnInfo.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/ListViewControl.h>
#include <rfb_win32/Win32Util.h>

namespace winvnc {
  
  class ControlPanel : rfb::win32::Dialog, rfb::win32::ListViewControl {
  public:
    ControlPanel(HWND hSTIcon) : Dialog(GetModuleHandle(nullptr)), ListViewControl(){
      m_hSTIcon = hSTIcon;
      stop_updating = false;
    };
    virtual bool showDialog();
    void initDialog() override;
    virtual bool onCommand(int cmd);
    void UpdateListView(ListConnInfo* LCInfo);
    HWND GetHandle() {return handle;};
    void SendCommand(DWORD command, int data);
    ~ControlPanel();
    ListConnInfo ListConnStatus;
  protected: 
    BOOL dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
    void getSelConnInfo();
    HWND m_hSTIcon;
    ListConnInfo ListConn;
    ListConnInfo ListSelConn;
    bool stop_updating;
  };
};

#endif  
