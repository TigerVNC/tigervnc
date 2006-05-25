# Microsoft Developer Studio Project File - Name="rfb_win32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=rfb_win32 - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rfb_win32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rfb_win32.mak" CFG="rfb_win32 - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rfb_win32 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "rfb_win32 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "rfb_win32 - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rfb_win32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\rfb_win32"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "WIN32" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "rfb_win32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\rfb_win32"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "WIN32" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "rfb_win32 - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rfb_win32___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "rfb_win32___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug_Unicode"
# PROP Intermediate_Dir "..\Debug_Unicode\rfb_win32"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"msvcwarning.h" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "WIN32" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "rfb_win32 - Win32 Release"
# Name "rfb_win32 - Win32 Debug"
# Name "rfb_win32 - Win32 Debug Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AboutDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\CKeyboard.cxx
# End Source File
# Begin Source File

SOURCE=.\CleanDesktop.cxx
# End Source File
# Begin Source File

SOURCE=.\Clipboard.cxx
# End Source File
# Begin Source File

SOURCE=.\CPointer.cxx
# End Source File
# Begin Source File

SOURCE=.\CurrentUser.cxx
# End Source File
# Begin Source File

SOURCE=.\DeviceContext.cxx
# End Source File
# Begin Source File

SOURCE=.\DeviceFrameBuffer.cxx
# End Source File
# Begin Source File

SOURCE=.\Dialog.cxx
# End Source File
# Begin Source File

SOURCE=.\DIBSectionBuffer.cxx
# End Source File
# Begin Source File

SOURCE=.\DynamicFn.cxx
# End Source File
# Begin Source File

SOURCE=.\EventManager.cxx
# End Source File
# Begin Source File

SOURCE=.\FolderManager.cxx
# End Source File
# Begin Source File

SOURCE=.\LaunchProcess.cxx
# End Source File
# Begin Source File

SOURCE=.\ListViewControl.cxx
# End Source File
# Begin Source File

SOURCE=.\LowLevelKeyEvents.cxx
# End Source File
# Begin Source File

SOURCE=.\MonitorInfo.cxx
# End Source File
# Begin Source File

SOURCE=.\MsgWindow.cxx
# End Source File
# Begin Source File

SOURCE=.\OSVersion.cxx
# End Source File
# Begin Source File

SOURCE=.\ProgressControl.cxx
# End Source File
# Begin Source File

SOURCE=.\RegConfig.cxx
# End Source File
# Begin Source File

SOURCE=.\Registry.cxx
# End Source File
# Begin Source File

SOURCE=.\ScaledDIBSectionBuffer.cxx
# End Source File
# Begin Source File

SOURCE=.\SDisplay.cxx
# End Source File
# Begin Source File

SOURCE=.\SDisplayCorePolling.cxx
# End Source File
# Begin Source File

SOURCE=.\SDisplayCoreWMHooks.cxx
# End Source File
# Begin Source File

SOURCE=.\Security.cxx
# End Source File
# Begin Source File

SOURCE=.\Service.cxx
# End Source File
# Begin Source File

SOURCE=.\SFileTransferManagerWin32.cxx
# End Source File
# Begin Source File

SOURCE=.\SFileTransferWin32.cxx
# End Source File
# Begin Source File

SOURCE=.\SInput.cxx
# End Source File
# Begin Source File

SOURCE=.\SocketManager.cxx
# End Source File
# Begin Source File

SOURCE=.\TCharArray.cxx
# End Source File
# Begin Source File

SOURCE=.\Threading.cxx
# End Source File
# Begin Source File

SOURCE=.\ToolBar.cxx
# End Source File
# Begin Source File

SOURCE=.\TsSessions.cxx
# End Source File
# Begin Source File

SOURCE=.\Win32Util.cxx
# End Source File
# Begin Source File

SOURCE=.\WMCursor.cxx
# End Source File
# Begin Source File

SOURCE=.\WMHooks.cxx
# End Source File
# Begin Source File

SOURCE=.\WMNotifier.cxx
# End Source File
# Begin Source File

SOURCE=.\WMPoller.cxx
# End Source File
# Begin Source File

SOURCE=.\WMShatter.cxx
# End Source File
# Begin Source File

SOURCE=.\WMWindowCopyRect.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AboutDialog.h
# End Source File
# Begin Source File

SOURCE=.\BitmapInfo.h
# End Source File
# Begin Source File

SOURCE=.\CKeyboard.h
# End Source File
# Begin Source File

SOURCE=.\CleanDesktop.h
# End Source File
# Begin Source File

SOURCE=.\Clipboard.h
# End Source File
# Begin Source File

SOURCE=.\CompatibleBitmap.h
# End Source File
# Begin Source File

SOURCE=.\ComputerName.h
# End Source File
# Begin Source File

SOURCE=.\CPointer.h
# End Source File
# Begin Source File

SOURCE=.\CurrentUser.h
# End Source File
# Begin Source File

SOURCE=.\DeviceContext.h
# End Source File
# Begin Source File

SOURCE=.\DeviceFrameBuffer.h
# End Source File
# Begin Source File

SOURCE=.\Dialog.h
# End Source File
# Begin Source File

SOURCE=.\DIBSectionBuffer.h
# End Source File
# Begin Source File

SOURCE=.\DynamicFn.h
# End Source File
# Begin Source File

SOURCE=.\EventManager.h
# End Source File
# Begin Source File

SOURCE=.\FolderManager.h
# End Source File
# Begin Source File

SOURCE=.\Handle.h
# End Source File
# Begin Source File

SOURCE=.\IconInfo.h
# End Source File
# Begin Source File

SOURCE=.\IntervalTimer.h
# End Source File
# Begin Source File

SOURCE=.\keymap.h
# End Source File
# Begin Source File

SOURCE=.\LaunchProcess.h
# End Source File
# Begin Source File

SOURCE=.\ListViewControl.h
# End Source File
# Begin Source File

SOURCE=.\LocalMem.h
# End Source File
# Begin Source File

SOURCE=.\LogicalPalette.h
# End Source File
# Begin Source File

SOURCE=.\LowLevelKeyEvents.h
# End Source File
# Begin Source File

SOURCE=.\ModuleFileName.h
# End Source File
# Begin Source File

SOURCE=.\MonitorInfo.h
# End Source File
# Begin Source File

SOURCE=.\MsgBox.h
# End Source File
# Begin Source File

SOURCE=.\MsgWindow.h
# End Source File
# Begin Source File

SOURCE=.\OSVersion.h
# End Source File
# Begin Source File

SOURCE=.\ProgressControl.h
# End Source File
# Begin Source File

SOURCE=.\RegConfig.h
# End Source File
# Begin Source File

SOURCE=.\Registry.h
# End Source File
# Begin Source File

SOURCE=.\ScaledDIBSectionBuffer.h
# End Source File
# Begin Source File

SOURCE=.\SDisplay.h
# End Source File
# Begin Source File

SOURCE=.\SDisplayCoreDriver.h
# End Source File
# Begin Source File

SOURCE=.\SDisplayCorePolling.h
# End Source File
# Begin Source File

SOURCE=.\SDisplayCoreWMHooks.h
# End Source File
# Begin Source File

SOURCE=.\Security.h
# End Source File
# Begin Source File

SOURCE=.\Service.h
# End Source File
# Begin Source File

SOURCE=.\SFileTransferManagerWin32.h
# End Source File
# Begin Source File

SOURCE=.\SFileTransferWin32.h
# End Source File
# Begin Source File

SOURCE=.\SInput.h
# End Source File
# Begin Source File

SOURCE=.\SocketManager.h
# End Source File
# Begin Source File

SOURCE=.\TCharArray.h
# End Source File
# Begin Source File

SOURCE=.\Threading.h
# End Source File
# Begin Source File

SOURCE=.\ToolBar.h
# End Source File
# Begin Source File

SOURCE=.\TrayIcon.h
# End Source File
# Begin Source File

SOURCE=.\TsSessions.h
# End Source File
# Begin Source File

SOURCE=.\Win32Util.h
# End Source File
# Begin Source File

SOURCE=.\WMCursor.h
# End Source File
# Begin Source File

SOURCE=.\WMHooks.h
# End Source File
# Begin Source File

SOURCE=.\WMNotifier.h
# End Source File
# Begin Source File

SOURCE=.\WMPoller.h
# End Source File
# Begin Source File

SOURCE=.\WMShatter.h
# End Source File
# Begin Source File

SOURCE=.\WMWindowCopyRect.h
# End Source File
# End Group
# End Target
# End Project
