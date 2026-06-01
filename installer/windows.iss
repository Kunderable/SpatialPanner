; Inno Setup script for Spatial Panner (Windows VST3 installer)
#define AppName "Spatial Panner"
#define AppVersion "1.0.0"

[Setup]
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=SpatialTools
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..
OutputBaseFilename=SpatialPanner-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName}

[Files]
Source: "..\build\SpatialPanner_artefacts\Release\VST3\Spatial Panner.vst3\*"; \
    DestDir: "{commoncf64}\VST3\Spatial Panner.vst3"; \
    Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Spatial Panner.vst3"

[Messages]
WelcomeLabel2=This will install {#AppName} {#AppVersion} (VST3) into your system VST3 folder.%n%nRescan plugins in your DAW after installing.
