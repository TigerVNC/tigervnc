
[Setup]
OutputDir=.
AppName=TigerVNC
AppVerName=TigerVNC 0.0.91
AppVersion=0.0.91
AppPublisher=TigerVNC project
AppPublisherURL=http://tigervnc.org
DefaultDirName={pf}\TigerVNC
DefaultGroupName=TigerVNC
LicenseFile=LICENCE.txt

[Files]
Source: "Release\winvnc4.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace; 
Source: "Release\wm_hooks.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace; 
Source: "Release\vncviewer.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace; 
Source: "Release\vncconfig.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace; 
Source: "README_BINARY.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENCE.txt"; DestDir: "{app}"; Flags: ignoreversion


[Icons]
Name: "{group}\TigerVNC Viewer"; FileName: "{app}\vncviewer.exe";
Name: "{group}\Listening TigerVNC Viewer"; FileName: "{app}\vncviewer.exe"; Parameters: "-listen";

Name: "{group}\VNC Server (User-Mode)\Run VNC Server"; FileName: "{app}\winvnc4.exe"; Parameters: "-noconsole";
Name: "{group}\VNC Server (User-Mode)\Configure VNC Server"; FileName: "{app}\vncconfig.exe"; Parameters: "-user";

Name: "{group}\VNC Server (Service-Mode)\Configure VNC Service"; FileName: "{app}\vncconfig.exe"; Parameters: "-noconsole -service";
Name: "{group}\VNC Server (Service-Mode)\Register VNC Service"; FileName: "{app}\winvnc4.exe"; Parameters: "-register";
Name: "{group}\VNC Server (Service-Mode)\Unregister VNC Service"; FileName: "{app}\winvnc4.exe"; Parameters: "-unregister";
Name: "{group}\VNC Server (Service-Mode)\Start VNC Service"; FileName: "{app}\winvnc4.exe"; Parameters: "-noconsole -start";
Name: "{group}\VNC Server (Service-Mode)\Stop VNC Service"; FileName: "{app}\winvnc4.exe"; Parameters: "-noconsole -stop";
Name: "{group}\License"; FileName: "{app}\LICENCE.txt";

[Tasks]
Name: installservice; Description: "&Register new TigerVNC Server as a system service"; GroupDescription: "Server configuration:"; 
Name: startservice; Description: "&Start or restart TigerVNC service"; GroupDescription: "Server configuration:";

[Run]
Filename: "{app}\winvnc4.exe"; Parameters: "-register"; Tasks: installservice
Filename: "net"; Parameters: "start winvnc4"; Tasks: startservice
