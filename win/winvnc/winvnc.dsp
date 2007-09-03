# Microsoft Developer Studio Project File - Name="winvnc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=winvnc - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "winvnc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "winvnc.mak" CFG="winvnc - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "winvnc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "winvnc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "winvnc - Win32 Debug Unicode" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "winvnc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\winvnc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "NDEBUG" /D "_CONSOLE" /D "WIN32" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 advapi32.lib user32.lib gdi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:console /machine:I386 /out:"../Release/winvnc4.exe"
# SUBTRACT LINK32 /profile
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Updating buildTime
PreLink_Cmds=cl /c /nologo /Fo..\Release\ /Fd..\Release\winvnc /MT buildTime.cxx
# End Special Build Tool

!ELSEIF  "$(CFG)" == "winvnc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\winvnc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_DEBUG" /D "_CONSOLE" /D "WIN32" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib user32.lib gdi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../Debug/winvnc4.exe" /fixed:no
# SUBTRACT LINK32 /profile /incremental:no
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Updating buildTime
PreLink_Cmds=cl /c /nologo /Fo..\Debug\ /Fd..\Debug\winvnc /MTd buildTime.cxx
# End Special Build Tool

!ELSEIF  "$(CFG)" == "winvnc - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "winvnc___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "winvnc___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug_Unicode"
# PROP Intermediate_Dir "..\Debug_Unicode\winvnc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /FI"msvcwarning.h" /D "_DEBUG" /D "_CONSOLE" /D "WIN32" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /I "../../common" /FI"rdr/msvcwarning.h" /D "_CONSOLE" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /out:"../Debug/winvnc4.exe" /fixed:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 advapi32.lib user32.lib gdi32.lib ws2_32.lib version.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /out:"..\Debug_Unicode/winvnc4.exe" /fixed:no
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=Updating buildTime
PreLink_Cmds=cl /c /nologo /Fo..\Debug_Unicode /Fd..\Debug_Unicode\winvnc /MTd buildTime.cxx
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "winvnc - Win32 Release"
# Name "winvnc - Win32 Debug"
# Name "winvnc - Win32 Debug Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\buildTime.cxx
# End Source File
# Begin Source File

SOURCE=.\ControlPanel.cxx
# End Source File
# Begin Source File

SOURCE=.\JavaViewer.cxx
# End Source File
# Begin Source File

SOURCE=.\ManagedListener.cxx
# End Source File
# Begin Source File

SOURCE=.\QueryConnectDialog.cxx
# End Source File
# Begin Source File

SOURCE=.\STrayIcon.cxx
# End Source File
# Begin Source File

SOURCE=.\VNCServerService.cxx
# End Source File
# Begin Source File

SOURCE=.\VNCServerWin32.cxx
# End Source File
# Begin Source File

SOURCE=.\winvnc.cxx
# End Source File
# Begin Source File

SOURCE=.\winvnc.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AddNewClientDialog.h
# End Source File
# Begin Source File

SOURCE=.\ControlPanel.h
# End Source File
# Begin Source File

SOURCE=.\JavaViewer.h
# End Source File
# Begin Source File

SOURCE=.\ManagedListener.h
# End Source File
# Begin Source File

SOURCE=.\QueryConnectDialog.h
# End Source File
# Begin Source File

SOURCE=.\STrayIcon.h
# End Source File
# Begin Source File

SOURCE=.\VNCServerService.h
# End Source File
# Begin Source File

SOURCE=.\VNCServerWin32.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\connecte.ico
# End Source File
# Begin Source File

SOURCE=.\connected.ico
# End Source File
# Begin Source File

SOURCE=.\icon_dis.ico
# End Source File
# Begin Source File

SOURCE=..\..\common\javabin\logo150x150.gif
# End Source File
# Begin Source File

SOURCE=.\winvnc.bmp
# End Source File
# Begin Source File

SOURCE=.\winvnc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\common\javabin\index.vnc
# End Source File
# Begin Source File

SOURCE=..\..\common\javabin\vncviewer.jar
# End Source File
# Begin Source File

SOURCE=.\winvnc4.exe.manifest
# End Source File
# End Target
# End Project
