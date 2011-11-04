include(head.html.m4)
define(SECT,install_windows)
include(navbar.html.m4)

TITLE(WINDOWS Installation)

SUBSECTION(Installer)
PARAGRAPH([
Run the executable, select a directory
to install armagetron into and enjoy. Start menu entries and a desktop shortcut will be created for you.
To uninstall, start the uninstaller from the start menu.
])

ifelse(,,,[
SUBSECTION(Updater)
PARAGRAPH([
Run the executable. It should find and update the previous version automatically.
])
])

ifelse(,,,[
SUBSECTION(Zip Archive)
PARAGRAPH([
If you have the .zip-version, unpack it to the place you want PROGTITLE to be
(using WinZip, for example) and create the desktop/startmenu shortcuts 
manually by right-dragging PROGNAME.exe from the installation directory.
])
PARAGRAPH([
To uninstall, simply delete the extracted directory and the shortcuts you made.
])
])

SECTION(Result)
define(DOCSTYLE_RESULT,windows)
include(install_result.html.m4)

include(sig.m4)
include(navbar.html.m4)
</body>
</html>

