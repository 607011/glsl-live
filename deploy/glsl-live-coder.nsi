!define VERSION "0.9.12-patch1"
!define APP "glsl-live-coder"
!define PUBLISHER "c't"
!define QTPATH "D:\Developer\Qt-5.3.1\5.3\msvc2012_opengl"

Name "${APP} ${VERSION}"
OutFile "${APP}-${VERSION}-setup.exe"
InstallDir $PROGRAMFILES\${APP}
InstallDirRegKey HKLM "Software\${PUBLISHER}\${APP}" "Install_Dir"
RequestExecutionLevel admin
SetCompressor lzma
ShowInstDetails show

Page license

  LicenseData ..\LICENSE.txt

Page directory

Page instfiles


;Section "Visual Studio 2012 edistributables"
;  File D:\Developer\Qt-5.3\vcredist\vcredist_sp1_x86.exe
;SectionEnd

Section "GLSLLive"

;  ReadRegStr $1 HKLM "SOFTWARE\Microsoft\VisualStudio\11.0\VC\VCRedist\x86" "Installed"
;  StrCmp $1 1 vcredist_installed
;  DetailPrint "Installing Visual Studio 2010 Redistributable (x86) ..."
;  ExecWait 'vcredist_sp1_x86.exe'
;vcredist_installed:

  SetOutPath $INSTDIR
  CreateDirectory $INSTDIR\plugins
  CreateDirectory $INSTDIR\plugins\imageformats
  CreateDirectory $INSTDIR\platforms
  CreateDirectory $INSTDIR\mediaservice
  CreateDirectory $INSTDIR\examples
  File v${VERSION}\GLSL-Live-Coder.exe
  File v${VERSION}\GLSL-Live-Coder.exe.embed.manifest
  File ${QTPATH}\bin\icudt52.dll
  File ${QTPATH}\bin\icuin52.dll
  File ${QTPATH}\bin\icuuc52.dll
  File ${QTPATH}\bin\Qt5Concurrent.dll
  File ${QTPATH}\bin\Qt5Core.dll
  File ${QTPATH}\bin\Qt5Gui.dll
  File ${QTPATH}\bin\Qt5Multimedia.dll
  File ${QTPATH}\bin\Qt5Network.dll
  File ${QTPATH}\bin\Qt5Widgets.dll
  File ${QTPATH}\bin\Qt5Script.dll
  File ${QTPATH}\bin\Qt5ScriptTools.dll
  File ${QTPATH}\bin\Qt5Xml.dll
  File ${QTPATH}\bin\Qt5OpenGL.dll
  File ..\LICENSE.txt
  File /oname=plugins\imageformats\qgif.dll ${QTPATH}\plugins\imageformats\qgif.dll
  File /oname=platforms\qminimal.dll ${QTPATH}\plugins\platforms\qminimal.dll
  File /oname=platforms\qwindows.dll ${QTPATH}\plugins\platforms\qwindows.dll
  File /oname=mediaservice\dsengine.dll ${QTPATH}\plugins\mediaservice\dsengine.dll
  File /oname=mediaservice\qtmedia_audioengine.dll ${QTPATH}\plugins\mediaservice\qtmedia_audioengine.dll
  File /oname=mediaservice\wmfengine.dll ${QTPATH}\plugins\mediaservice\wmfengine.dll
  WriteUninstaller $INSTDIR\uninstall.exe
  SetOutPath $INSTDIR\examples
  File ..\examples\*.xml
SectionEnd


Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\${APP}"
  CreateShortCut "$SMPROGRAMS\${APP}\${APP} ${VERSION}.lnk" "$INSTDIR\${APP}.exe"
  CreateShortcut "$SMPROGRAMS\${APP}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd


Section "Uninstall"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"
  DeleteRegKey HKLM SOFTWARE\${APP}

  Delete $INSTDIR\lolQt.exe
  Delete $INSTDIR\lolQt.exe.embed.manifest
  Delete $INSTDIR\LICENSE
  Delete $INSTDIR\mencoder.exe
  Delete $INSTDIR\big_noodle_titling.ttf
  Delete $INSTDIR\icudt52.dll
  Delete $INSTDIR\icuin52.dll
  Delete $INSTDIR\icuuc52.dll
  Delete $INSTDIR\Qt5Concurrent.dll
  Delete $INSTDIR\Qt5Core.dll
  Delete $INSTDIR\Qt5Gui.dll
  Delete $INSTDIR\Qt5Multimedia.dll
  Delete $INSTDIR\Qt5Network.dll
  Delete $INSTDIR\Qt5Widgets.dll
  Delete $INSTDIR\Qt5Script.dll
  Delete $INSTDIR\Qt5ScriptTools.dll
  Delete $INSTDIR\Qt5Xml.dll
  Delete $INSTDIR\Qt5OpenGL.dll
  Delete $INSTDIR\uninstall.exe

  Delete $INSTDIR\mediaservice\dsengine.dll
  Delete $INSTDIR\mediaservice\qtmedia_audioengine.dll
  Delete $INSTDIR\mediaservice\wmfengine.dll
  RMDir $INSTDIR\mediaservice

  Delete $INSTDIR\platforms\qminimal.dll
  Delete $INSTDIR\platforms\qwindows.dll
  RMDir $INSTDIR\platforms

  Delete $INSTDIR\plugins\imageformats\qgif.dll
  RMDir $INSTDIR\plugins\imageformats

  RMDir $INSTDIR\plugins

  Delete $INSTDIR\examples\*.*
  RMDir $INSTDIR\examples
  RMDir $INSTDIR

  Delete "$SMPROGRAMS\glsl-live-coder\*.*"
  RMDir "$SMPROGRAMS\glsl-live-code"
  RMDir "$INSTDIR"
SectionEnd
