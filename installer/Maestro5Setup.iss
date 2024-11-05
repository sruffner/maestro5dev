; --Maestro5Setup.iss --
; Inno Setup script to generate a Windows installer for the 64-bit Windows 11/RTX64 4.5 application Maestro 5.x.
; 
; NOTES:
; 1) This is a simple installer. It simply copies the Maestro program and support files to a specified
; installation directory, then creates the registry key HKLM\SOFTWARE\HHMI-LisbergerLab\Maestro and stores
; the full pathname for the installation directory in a REG_SZ-valued entry "Home" under that key.
; 2) If the installer detects an existing Maestro installation (by checking for the existence of the Maestro
; key, along with entries "Home" and "Version"), it will simply update the existing installation in place.
; In this case, the user does not have the opportunity to select the installation directory.
; 3) An uninstaller is also included. If Maestro is already installed and you wish to move the installation
; to a different directory, you must uninstall Maestro, then do a fresh install.
; 4) As of Sep 2019, the entry "SetDOBusyWaits" was added under the Maestro registry key. This REG_SZ value
; lists the three busy wait times for delivering a digital output command to latched external devices. A
; new custom page lets the user set/modify these wait times, which are restricted to [0..20] microseconds.
; 5) The installer will check that the host is 64-bit and that OS version matches Win10 1607 LTSB.
; 6) The installer does NOT register the Maestro experiment document .CXE file type with the Maestro app.
; 7) Since it adds a key to HKLM and since we want any user to be able to run Maestro, the installation
; requires admin privileges.


[Setup]
AppId={{42FF9641-8AA1-4A53-908B-7CD0CA02FFF8}}
AppName=Maestro
AppVersion={code:GetMaestroVersion}
AppVerName=Maestro {code:GetMaestroVersion}
DefaultDirName={commonpf64}\Maestro5
DefaultGroupName=Maestro 5.x
UninstallDisplayIcon={app}\misc\Maestro5Setup.ico
Compression=lzma2/max
SolidCompression=yes
OutputDir=.
OutputBaseFilename=Maestro5Setup
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
ArchitecturesAllowed=x64os
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64os

; always want the initial welcome page to appear
DisableWelcomePage=no

; we let user select installation directory for a brand-new install but not an update
; the user does not control the program group
DisableDirPage=auto
DisableProgramGroupPage=yes
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes

AllowNetworkDrive=no
AllowUNCPath=no
CloseApplications=yes
RestartApplications=no
; Windows 11 Version 23H2
MinVersion=10.0.22631	
PrivilegesRequired=admin
SetupIconFile=files/Maestro5Setup.ico
SetupLogging=yes
Uninstallable=yes
UninstallFilesDir={app}\misc

AppCopyright=Copyright (C) 2003-2025 Duke University (Lisberger Lab)/Scott Ruffner Scientific Computing
AppPublisher=Scott Ruffner Scientific Computing
AppPublisherURL=https://sites.google.com/a/srscicomp.com/maestro
AppSupportURL=https://sites.google.com/a/srscicomp.com/maestro
AppUpdatesURL=https://sites.google.com/a/srscicomp.com/maestro/downloads
VersionInfoVersion=1.0

[Files]
Source: "files\maestroGUI.exe"; DestDir: "{app}"; DestName: "maestro.exe"; Flags: ignoreversion
Source: "files\cxdriver.rtss"; DestDir: "{app}"; Flags: ignoreversion
Source: "files\eyelink_core64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "files\drivers\*.*"; DestDir: "{app}\drivers"; Flags: ignoreversion
Source: "files\version.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "files\Maestro5Setup.ico"; DestDir: "{app}\misc"; Flags: ignoreversion

; This removes the installation directory unless the user added something that wasn't part of the original installation.
[UninstallDelete]
Type: dirifempty; Name: {app}

[Icons]
Name: "{group}\Maestro"; Filename: "{app}\maestro.exe"; AppUserModelID: "SRSC.Maestro5"; Flags: createonlyiffileexists
Name: "{group}\Maestro Online Guide"; Filename: "https://sites.google.com/a/srscicomp.com/maestro"; Flags: preventpinning
Name: "{group}\Uninstall Maestro"; Filename: "{uninstallexe}"; Flags: createonlyiffileexists preventpinning

[Registry]
Root: HKLM; Subkey: "SOFTWARE\HHMI-LisbergerLab"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "SOFTWARE\HHMI-LisbergerLab\Maestro"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\HHMI-LisbergerLab\Maestro"; ValueType: string; ValueName: "Home"; ValueData: "{app}"
Root: HKLM; Subkey: "SOFTWARE\HHMI-LisbergerLab\Maestro"; ValueType: string; ValueName: "Version"; ValueData: "{code:GetMaestroVersion}"
Root: HKLM; Subkey: "SOFTWARE\HHMI-LisbergerLab\Maestro"; ValueType: string; ValueName: "SetDOBusyWaits"; ValueData: "{code:GetDOBusy}";

[Code]
const
  MaestroVersionStr = '5.0.1';

var
  AlreadyInstalled: Boolean;
  ExistingVersionStr: String;
  DOBusyWaitsPage: TInputQueryWizardPage;
  DOBusy1, DOBusy2, DOBusy3 : Extended;

function GetMaestroVersion(Param: String) : String;
begin
  Result := MaestroVersionStr;
end;

function GetDOBusy(Param: String) : String;
begin
  Result := Format('%.1f,%.1f,%.1f', [DOBusy1, DOBusy2, DOBusy3]);
end;


// Wrapper for StrToFloat that returns default value if S does not represent a valid floating-point number. Recognizes
// only a leading '-', a single '.' decimal. No leading zeros. Scientific notation not supported.
function StrToFloatEx(S: String; Def: Extended) : Extended;
var
  ok, gotDot: Boolean;
  i, len: Integer;
  ordinal: Byte;

begin
  Result := Def;
  gotDot := False;
  len := Length(S);
  ok := (len > 0);
  i := 1;
  while((i<=len) and (ok)) do begin
    if(S[i]='-') then begin
      ok := (i=1);
    end else if(S[i]='.') then begin
      ok := not gotdot;
      gotdot := True;
    end else if(S[i]='0') then begin
      if((i=1) or ((i=2) and (S[1]='-'))) then begin
        ok := (len = i) or ((len>i) and (S[i+1]='.'));
      end else
        ok := True;
    end else begin
      ordinal := Ord(S[i]) - Ord('0');
      ok:= (ordinal >= 0) and (ordinal <= 9);
    end;

    i := i + 1;
  end;

  if(ok) then begin
    Result := StrToFloat(S);
  end;
end;


// Customization of the installer wizard:
// (1) Description on the Welcome Page indicates what installer will do -- update an existing installation or install a
//     fresh one. The description is set in WizardForm.WelcomeLabel2.Caption.
// (2) Add a custom input query page that lets the user set the DO command busy wait times in us.
procedure InitializeWizard;
var
  S : String;
begin
  if(AlreadyInstalled) then begin
     S := 'This will UPDATE the existing Maestro installation from version ' + ExistingVersionStr + ' to ' + MaestroVersionStr;
  end else
     S := 'This will install Maestro ' + MaestroVersionStr + ' on your computer';
  S := S + '.' + Chr(10) + Chr(10) + 'It is recommended that you close all other applications before continuing.';
  S := S + Chr(10) + Chr(10) + 'Click Next to continue, or Cancel to exit Setup.';
  WizardForm.WelcomeLabel2.Caption := S;

  S := 'Maestro communicates with external digital devices by writing commands on the digital output port DO<15..0>. ';
  S := S + 'The DO command has three stages, with a software "busy wait" after each stage. Adjust busy wait times ';
  S := S + 'if needed to ensure your DIO interface does not miss any DO commands. Then click "Next".';
  DOBusyWaitsPage := CreateInputQueryPage(wpSelectDir, 'Registry Setting', 'Digital Output Command Busy Waits', S);
  DOBusyWaitsPage.Add('Delay after DO write [0..20 microseconds]', False);
  DOBusyWaitsPage.Add('Delay after DataReady lowered [0..20 microseconds]:', False);
  DOBusyWaitsPage.Add('Delay after DataReady raised [0..20 microseconds]:', False);

  DOBusyWaitsPage.Values[0] := FloatToStr(DOBusy1);
  DOBusyWaitsPage.Values[1] := FloatToStr(DOBusy2);
  DOBusyWaitsPage.Values[2] := FloatToStr(DOBusy3);
end;

// The custom page setting the DO command busy wait times is never skipped
function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
end;

// Check invalid values entered for DO busy wait times and force user to enter valid values
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = DOBusyWaitsPage.ID then begin
    Result := True;
    DOBusy1 := StrToFloatEx(Trim(DOBusyWaitsPage.Values[0]), -1.0);
    if((DOBusy1 < 0) or (DOBusy1 > 20)) then begin
      DOBusy1 := 0.5;
      Result := False;
    end;  
    DOBusy2 := StrToFloatEx(Trim(DOBusyWaitsPage.Values[1]), -1.0);
    if((DOBusy2 < 0) or (DOBusy2 > 20)) then begin
      DOBusy2 := 2.5;
      Result := False;
    end;  
    DOBusy3 := StrToFloatEx(Trim(DOBusyWaitsPage.Values[2]), -1.0);
    if((DOBusy3 < 0) or (DOBusy3 > 20)) then begin
      DOBusy3 := 0.5;
      Result := False;
    end;
    if not Result then begin
      MsgBox('You must enter valid busy wait times in [0..20], in microseconds', mbError, MB_OK);
    end; 
  end else
    Result := True;
end;

// Include the DO busy wait settings in the 'Ready Memo', along with the other stuff
function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
var
  S: String;
begin
  S := '';
  S := S + MemoDirInfo + NewLine;
  S := S + MemoGroupInfo + NewLine;
  S := S + 'DO Command Busy Waits (microsecs): ' + GetDOBusy('') + NewLine;
  Result := S;
end;

// Check registry for Maestro key in HKLM, indicating an existing installaion. If Home or Version entries are missing, then abort installation.
// Otherwise, initialize DO busy wait times to default values or use the values parsed from the SetDOBusyWaits entry (if present)
function InitializeSetup(): Boolean;
var
  dirName, errMsg, strDOBusy, strWait : String;
  i : Integer;

begin
  Result := True;
  AlreadyInstalled := False;
  ExistingVersionStr := '';
  DOBusy1 := 0.5;
  DOBusy2 := 2.5;
  DOBusy3 := 0.5;

  if RegKeyExists(HKLM, 'SOFTWARE\HHMI-LisbergerLab\Maestro') then begin
    if RegQueryStringValue(HKLM, 'SOFTWARE\HHMI-LisbergerLab\Maestro', 'Home', dirName) and
       RegQueryStringValue(HKLM, 'SOFTWARE\HHMI-LisbergerLab\Maestro', 'Version', ExistingVersionStr) then begin
      AlreadyInstalled := True;
    end else begin
      errMsg := 'Detected missing entries in Windows registry while checking for existing installation. Cannot continue.';
      MsgBox(errMsg, mbError, MB_OK);
      Result:= False;
    end;
  end;

  if AlreadyInstalled and
     RegQueryStringValue(HKLM, 'SOFTWARE\HHMI-LisbergerLab\Maestro', 'SetDOBusyWaits', strDOBusy) then begin
    i := Pos(',', strDOBusy);
    if(i > 1) then begin
      strWait := Trim(Copy(strDOBusy, 1, i-1));
      Delete(strDOBusy, 1, i);
      DOBusy1 := StrToFloatEx(strWait, -1.0);
      if((DOBusy1 < 0.0) or (DOBusy1 > 20.0)) then begin
        DOBusy1 := 0.5;
      end;
      i := Pos(',', strDOBusy);
    end;
    if(i > 1) then begin
      strWait := Trim(Copy(strDOBusy, 1, i-1));
      Delete(strDOBusy, 1, i);
      DOBusy2 := StrToFloatEx(strWait, -1.0);
      if((DOBusy2 < 0.0) or (DOBusy2 > 20.0)) then begin
        DOBusy2 := 2.5;
      end;
      i := Length(strDOBusy);
    end else
      i := 0;
    end;
    if(i > 0) then begin
      DOBusy3 := StrToFloatEx(Trim(strDOBusy), -1.0);
      if((DOBusy3 < 0.0) or (DOBusy3 > 20.0)) then begin
        DOBusy3 := 0.5;
      end;
    end;
  end;
end.

