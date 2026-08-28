#ifndef AppVersion
  #error AppVersion must be supplied, for example /DAppVersion=v0.3.1-alpha.1
#endif

#ifndef NumericVersion
  #error NumericVersion must be supplied, for example /DNumericVersion=0.3.1.1
#endif

#ifndef PayloadDir
  #error PayloadDir must point to the extracted Windows package root
#endif

#ifndef BuildOutputDir
  #define BuildOutputDir "..\..\dist"
#endif

#define PayloadName "MGS4Ultra120-" + AppVersion + "-windows"

[Setup]
AppId={{5D5DA51F-D115-4B98-A72B-4715733857DF}
AppName=MGS4 Ultra120
AppVersion={#AppVersion}
AppVerName=MGS4 Ultra120 {#AppVersion}
AppPublisher=drbermejor
AppPublisherURL=https://github.com/drbermejor/mgs4Ultra120
AppSupportURL=https://github.com/drbermejor/mgs4Ultra120/issues
AppUpdatesURL=https://github.com/drbermejor/mgs4Ultra120/releases
VersionInfoVersion={#NumericVersion}
VersionInfoCompany=drbermejor
VersionInfoDescription=MGS4 Ultra120 unsigned Windows installer
VersionInfoProductName=MGS4 Ultra120
VersionInfoProductVersion={#NumericVersion}
OutputDir={#BuildOutputDir}
OutputBaseFilename=MGS4Ultra120-{#AppVersion}-windows-setup
Compression=lzma2/ultra64
SolidCompression=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
WizardStyle=modern
DisableWelcomePage=no
DisableDirPage=yes
DefaultDirName={localappdata}\Programs\MGS4 Ultra120
DefaultGroupName=MGS4 Ultra120
DisableProgramGroupPage=auto
DisableReadyPage=no
DisableFinishedPage=no
CreateAppDir=yes
Uninstallable=not IsSmokeTest
InfoBeforeFile=UNSIGNED_WARNING.txt
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "Test-WindowsPackage.ps1"; DestDir: "{tmp}"; Flags: ignoreversion deleteafterinstall

[Icons]
Name: "{group}\MGS4 Ultra120 Setup"; Filename: "{app}\MGS4Ultra120-Setup.cmd"; WorkingDir: "{app}"; Check: not IsSmokeTest
Name: "{group}\Uninstall MGS4 Ultra120"; Filename: "{uninstallexe}"; Check: not IsSmokeTest
Name: "{autodesktop}\MGS4 Ultra120 Setup"; Filename: "{app}\MGS4Ultra120-Setup.cmd"; WorkingDir: "{app}"; Tasks: desktopicon; Check: not IsSmokeTest

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoLogo -NoProfile -ExecutionPolicy Bypass -File ""{tmp}\Test-WindowsPackage.ps1"" -PackageDir ""{app}"""; WorkingDir: "{app}"; StatusMsg: "Validating the bundled Windows package..."; Flags: waituntilterminated; Check: IsSmokeTest
Filename: "{app}\MGS4Ultra120-Setup.cmd"; Description: "Open MGS4 Ultra120 Setup"; WorkingDir: "{app}"; Flags: postinstall nowait skipifsilent; Check: not IsSmokeTest

[Code]
function IsSmokeTest: Boolean;
begin
  Result := CompareText(ExpandConstant('{param:SMOKETEST|0}'), '1') = 0;
end;

function InitializeUninstall: Boolean;
var
  ResultCode: Integer;
  PowerShellPath: String;
  CleanupScript: String;
  Arguments: String;
begin
  Result := True;
  CleanupScript := ExpandConstant(
    '{app}\scripts\windows\uninstall-installed-package.ps1');
  if not FileExists(CleanupScript) then
    exit;

  PowerShellPath := ExpandConstant(
    '{sys}\WindowsPowerShell\v1.0\powershell.exe');
  Arguments := '-NoLogo -NoProfile -ExecutionPolicy Bypass -File "' +
    CleanupScript + '"';
  if (not Exec(PowerShellPath, Arguments, '', SW_HIDE,
               ewWaitUntilTerminated, ResultCode)) or (ResultCode <> 0) then
  begin
    MsgBox('The game patch could not be removed safely. Close MGS4 and retry. ' +
      'The installed setup files have been kept so backups are not lost.',
      mbError, MB_OK);
    Result := False;
  end;
end;
