; Armagetron Advanced Dedicated Server Setup Script
!define PRODUCT_NAME "Armagetron Advanced Dedicated Server"
!define PRODUCT_VERSION "CVS"
!define PRODUCT_PUBLISHER "Armagetron Advanced Team"
!define PRODUCT_WEB_SITE "http://armagetronad.sf.net"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\armagetronad_dedicated.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "banner.bmp"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"
!define MUI_WELCOMEPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_TITLE_3LINES

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "COPYING.txt"
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Spanish"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "armagetronad-dedicated-${PRODUCT_VERSION}.win32.exe"
InstallDir "$PROGRAMFILES\Armagetron Advanced Dedicated"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Section "MainSection" SEC01

  SetOutPath "$INSTDIR"
  SetOverwrite try
  File "Armagetron Forums.url"

  # install desktop entry for current user
  CreateShortCut "$DESKTOP\Armagetron Advanced Server.lnk" "$INSTDIR\armagetronad_dedicated.exe"

  # install start menu for all users
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\Armagetron Advanced"
  CreateShortCut "$SMPROGRAMS\Armagetron Advanced\Armagetron Forums.lnk" "$INSTDIR\Armagetron Forums.url"
  File "armagetronad_dedicated.exe"
  CreateShortCut "$SMPROGRAMS\Armagetron Advanced\Armagetron Advanced Server.lnk" "$INSTDIR\armagetronad_dedicated.exe"
  SetOutPath "$INSTDIR\config"
  File ".\config\*.cfg"
  File ".\config\master.srv"
  SetOutPath "$INSTDIR\config\examples"
  File ".\config\examples\*.cfg"
  SetOutPath "$INSTDIR\config\examples\cvs_test"
  File ".\config\examples\cvs_test\*.cfg"
  SetOutPath "$INSTDIR"
  File "*.txt"
  SetOutPath "$INSTDIR\doc"
  File ".\doc\*.html"
  CreateShortCut "$SMPROGRAMS\Armagetron Advanced\Documentation.lnk" "$INSTDIR\doc\index.html"
  SetOutPath "$INSTDIR\doc\net"
  File ".\doc\net\*.html"
  SetOutPath "$INSTDIR\language"
  File ".\language\*.*"
  SetOutPath "$INSTDIR"
  File "zlib1.dll"
  File "iconv.dll"
  File "libxml2.dll"
  SetOutPath "$INSTDIR\models"
  File ".\models\*.mod"
  SetOutPath "$INSTDIR\resource\included"
  File /r ".\resource\included\*.*"
  SetOutPath "$INSTDIR\sound"
  File ".\sound\*.wav"
  SetOutPath "$INSTDIR\textures"
  File ".\textures\*.png"
  File ".\textures\*.jpg"
  #SetOutPath "$INSTDIR\var"
  #File ".\var\README.txt"
SectionEnd

Section -AdditionalIcons
  CreateShortCut "$SMPROGRAMS\Armagetron Advanced\Uninstall Armagetron Advanced Server.lnk" "$INSTDIR\uninst_dedicated.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst_dedicated.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\armagetronad_dedicated.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst_dedicated.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\armagetronad_dedicated.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

Function un.onInit
!insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Section Uninstall
  # uninstall for all users
  SetShellVarContext all

  Delete "$INSTDIR\*.txt"
  Delete "$INSTDIR\*.exe"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\var\README.txt"
  Delete "$INSTDIR\var\stdout.txt"

  RMDir /r "$INSTDIR\textures"
  RMDir /r "$INSTDIR\sound"
  RMDir /r "$INSTDIR\resource\included"
  RMDir /r "$INSTDIR\models"
  RMDir /r "$INSTDIR\language"
  RMDir /r "$INSTDIR\doc"
  RMDir /r "$INSTDIR\config\examples"
  Delete "$INSTDIR\config\settings_visual.cfg"
  Delete "$INSTDIR\config\settings_dedicated.cfg"
  Delete "$INSTDIR\config\settings.cfg"
  Delete "$INSTDIR\config\master.srv"
  Delete "$INSTDIR\config\default.cfg"
  Delete "$INSTDIR\config\aiplayers.cfg"
  Delete "$INSTDIR\armagetronad.exe"
  Delete "$INSTDIR\Armagetron Forums.url"

  RMDir "$INSTDIR\var"
  RMDir "$INSTDIR\resource"
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\Armagetron Advanced\Uninstall Armagetron Advanced Server.lnk"
  Delete "$SMPROGRAMS\Armagetron Advanced\Documentation.lnk"
  Delete "$DESKTOP\Armagetron Advanced Server.lnk"
  Delete "$SMPROGRAMS\Armagetron Advanced\Armagetron Advanced Server.lnk"
  Delete "$SMPROGRAMS\Armagetron Advanced\Armagetron Forums.lnk"

  RMDir "$SMPROGRAMS\Armagetron Advanced"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
