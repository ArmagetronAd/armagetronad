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

SECTION(OpenGL Driver)
PARAGRAPH([
PROGTITLE uses OpenGL for the 3D graphics. A software implementation of OpenGL comes
with all Windows versions and PROGTITLE can use it. Of course, if you have a 3D accelerator card,
you want to use that; usually, when you installed the driver for your card, an accelerated implementation
of OpenGL was registered with Windows.
])
PARAGRAPH([
If you experience problems with the graphics or if PROGTITLE does not even start correctly, try
installing the most recent drivers for your graphics card you can find.])

SUBSUBSECTION(Special notes for VooDoo addon cards (i.e. VooDoo 1,VooDoo 2):)
PARAGRAPH([
  A small problem here: the standard OPENGL32.DLL has no hardware
  acceleration. Instead, 3DFX supplies you with a file called 
  3dfxVGL.DLL [or 3dfxOGL.DLL] (found in your windows\system folder).
  Simply copy   (drag it with the right mouse button and select "copy" from the
   pop-up-menu) it into the folder "PROGNAME.exe" is in and rename it 
  there to "OPENGL32.DLL". Then, "PROGNAME.exe" will use this file 
  instead of Microsoft's standard OPENGL32.DLL. If you cannot find 
  3dfxVGL.DLL or 3dfxOGL.DLL, you should first check whether you can
  find ANY .DLL files in the folder; if not, you'll have to enable
  the item "show all files" (or similar) in the folder options.
  <br>
  If you have problems with that procedure or get crashes in glide3x.*, 
  ELINK(WEBBASE/download/opengl32.zip,download this working version) for VooDoo 2 (maybe 1) 
  and unpack it into your PROGTITLE directory. According to the driver, you
  need at least Glide 2.56 installed; it really should work if you use
  the driver version 3.02.02 (or higher?).
])

You need to be in 16 bit colour depth mode to run PROGTITLE on some cards
(Riva 128, some VooDoos).

include(sig.m4)
include(navbar.html.m4)
</body>
</html>

