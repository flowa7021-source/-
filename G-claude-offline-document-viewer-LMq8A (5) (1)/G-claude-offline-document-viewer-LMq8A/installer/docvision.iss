; DocVision Inno Setup Installer
; Download Inno Setup: https://jrsoftware.org/isinfo.php
; Compile: iscc docvision.iss

#define MyAppName "DocVision"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "DocVision Project"
#define MyAppURL "https://github.com/docvision"
#define MyAppExeName "DocVision.exe"

; Build directory — override via command line:
;   iscc /DBuildDir=C:\path\to\build\Release docvision.iss
#ifndef BuildDir
  #define BuildDir "..\build\Release"
#endif

[Setup]
AppId={{A7D3F2E1-8B4C-4E5F-9A6D-1C2B3E4F5A6B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile=..\LICENSE
OutputDir=..\build\installer
OutputBaseFilename=DocVision-{#MyAppVersion}-Setup-x64
SetupIconFile=..\resources\icons\docvision.ico
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
WizardStyle=modern
PrivilegesRequired=admin
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Setup
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
DisableProgramGroupPage=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "fileassoc_pdf"; Description: "Associate .pdf files"; GroupDescription: "File associations:"; Flags: checkedonce
Name: "fileassoc_djvu"; Description: "Associate .djvu files"; GroupDescription: "File associations:"; Flags: checkedonce
Name: "fileassoc_cbz"; Description: "Associate .cbz files"; GroupDescription: "File associations:"; Flags: checkedonce
Name: "fileassoc_epub"; Description: "Associate .epub files"; GroupDescription: "File associations:"; Flags: checkedonce

[Files]
; Main executable
Source: "{#BuildDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Runtime DLLs (vcpkg dependencies, etc.)
Source: "{#BuildDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

; Application icon
Source: "..\resources\icons\docvision.ico"; DestDir: "{app}"; Flags: ignoreversion

; Config templates
Source: "..\resources\default_settings.json"; DestDir: "{app}\config"; Flags: ignoreversion
Source: "..\resources\default_hotkeys.json"; DestDir: "{app}\config"; Flags: ignoreversion

; License
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

; Tesseract OCR data (if present)
Source: "..\tessdata\*.traineddata"; DestDir: "{app}\tessdata"; Flags: ignoreversion skipifsourcedoesntexist

[Dirs]
Name: "{app}\config"
Name: "{app}\logs"
Name: "{app}\cache"
Name: "{app}\cache\ocr"
Name: "{app}\tessdata"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; PDF association
Root: HKCR; Subkey: ".pdf\OpenWithProgids"; ValueType: string; ValueName: "DocVision.pdf"; ValueData: ""; Tasks: fileassoc_pdf
Root: HKCR; Subkey: "DocVision.pdf"; ValueType: string; ValueName: ""; ValueData: "PDF Document - DocVision"; Tasks: fileassoc_pdf
Root: HKCR; Subkey: "DocVision.pdf\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: fileassoc_pdf
Root: HKCR; Subkey: "DocVision.pdf\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc_pdf

; DjVu association
Root: HKCR; Subkey: ".djvu\OpenWithProgids"; ValueType: string; ValueName: "DocVision.djvu"; ValueData: ""; Tasks: fileassoc_djvu
Root: HKCR; Subkey: ".djv\OpenWithProgids"; ValueType: string; ValueName: "DocVision.djvu"; ValueData: ""; Tasks: fileassoc_djvu
Root: HKCR; Subkey: "DocVision.djvu"; ValueType: string; ValueName: ""; ValueData: "DjVu Document - DocVision"; Tasks: fileassoc_djvu
Root: HKCR; Subkey: "DocVision.djvu\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: fileassoc_djvu
Root: HKCR; Subkey: "DocVision.djvu\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc_djvu

; CBZ association
Root: HKCR; Subkey: ".cbz\OpenWithProgids"; ValueType: string; ValueName: "DocVision.cbz"; ValueData: ""; Tasks: fileassoc_cbz
Root: HKCR; Subkey: "DocVision.cbz"; ValueType: string; ValueName: ""; ValueData: "CBZ Comic Archive - DocVision"; Tasks: fileassoc_cbz
Root: HKCR; Subkey: "DocVision.cbz\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: fileassoc_cbz
Root: HKCR; Subkey: "DocVision.cbz\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc_cbz

; EPUB association
Root: HKCR; Subkey: ".epub\OpenWithProgids"; ValueType: string; ValueName: "DocVision.epub"; ValueData: ""; Tasks: fileassoc_epub
Root: HKCR; Subkey: "DocVision.epub"; ValueType: string; ValueName: ""; ValueData: "EPUB E-Book - DocVision"; Tasks: fileassoc_epub
Root: HKCR; Subkey: "DocVision.epub\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: fileassoc_epub
Root: HKCR; Subkey: "DocVision.epub\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc_epub

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\logs"
Type: filesandordirs; Name: "{app}\cache"
Type: dirifempty; Name: "{app}\config"
Type: dirifempty; Name: "{app}\tessdata"
Type: dirifempty; Name: "{app}"

[Code]
// Notify Windows that file associations changed
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Refresh shell icon cache
    RegWriteStringValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Explorer', 'DocVisionInstalled', 'yes');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // Clean up file association registry on uninstall
    RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'DocVision.pdf');
    RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'DocVision.djvu');
    RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'DocVision.cbz');
    RegDeleteKeyIncludingSubkeys(HKEY_CLASSES_ROOT, 'DocVision.epub');
    RegDeleteValue(HKEY_CLASSES_ROOT, '.pdf\OpenWithProgids', 'DocVision.pdf');
    RegDeleteValue(HKEY_CLASSES_ROOT, '.djvu\OpenWithProgids', 'DocVision.djvu');
    RegDeleteValue(HKEY_CLASSES_ROOT, '.djv\OpenWithProgids', 'DocVision.djvu');
    RegDeleteValue(HKEY_CLASSES_ROOT, '.cbz\OpenWithProgids', 'DocVision.cbz');
    RegDeleteValue(HKEY_CLASSES_ROOT, '.epub\OpenWithProgids', 'DocVision.epub');
    RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Explorer', 'DocVisionInstalled');
  end;
end;
