include(head.html.m4)
define(SECT,install_linux)
include(navbar.html.m4)

TITLE(LINUX Installation)

SECTION(General comments)
PARAGRAPH([
All systemwide installation has to be carried out as root. 
Installation from a source archive is recommended,
as PROGTITLE will be tailored for your system and missing libraries will be identified and
pointed out to you. 
Only as a last resort, you should choose the binary archives. 
The binary RPMs and Debian packages are a reasonable choice 
if your system supports them and you can get hold of
the corresponding library packages in the right versions.
])
PARAGRAPH([
It is very important that Armagetron and all of its dependencies are built with the same
compiler. The different versions of gcc tend to be slightly binary incompatible with each other,
and on complex projects like this one these small glitches tend to cause unexplainable problems.
The distributed binaries are compiled with gcc 3.3, and we test source compatibility with 3.4 
and 4.0 regularly. Yes, theoretically the binary interface for C code (ABI)
did not change, but be prepared for problems nevertheless.
])

SECTION(Required libraries,libs)

ULIST([
ITEM(ELINK(http://www.xmlsoft.org/,LibXML2) version 2.6.0 (some binaries: 2.6.12) or later for map file parsing)
ITEM([OpenGL or ELINK(mesa3d.sourceforge.net/,Mesa) for rendering])
ITEM(ELINK(www.libsdl.org,SDL) version 1.2.x (Simple Direct Media Layer) for input and sound output)
ITEM(ELINK(www.libsdl.org/projects/SDL_image/index.html, SDL_image)
version 1.2.x for the textures
which itself needs the libs)
ITEM(ELINK(www.libpng.org/pub/png/pngcode.html,pnglib and zlib)
(they should be included in your distribution))
ITEM(Optional: ELINK(www.libsdl.org/projects/SDL_mixer/,SDL_mixer))
ITEM([Binary versions only: libstdc++ 5.0.7 (any 5.x.y or whatever came with
your System if it uses GCC 3.2 or higher should do). Use 
ELINK([rpmfind.net/linux/rpm2html/search.php?query=libstdc%2B%2B&submit=Search+...&system=&arch=],rpmfind)
or
ELINK([www.google.com/search?hl=en&lr=&q=libstdc%2B%2B+5.0.7&btnG=Search],google)
to find a RPM with that library for your system. Sorry, Debian users: I don't know what you need to do; use the
source packages instead.])
])

PARAGRAPH(,[
You may take a look at the ELINK(WEBBASE/compatibility.html,compatibility table)
if you want to check how likely it is for Armagetron to work on your PC.
])

SECTION(Installation)

SUBSECTION(Source archive,source)
PARAGRAPH([
Unpack the archive and change into the unpacked directory; type 
CMDLINE(./configure
gmake install)
Everything will be copied to FILE(PREFIX/games/armagetron). 
Should there already be a version, it will be overwritten (see the section on
LINK([install_linux.html#multiple],[multiple versions]) if you want to keep your old
version around).
If FILE(gmake) is not found on your system, try 
FILE(make); however, only GNU make is supported.
To build the dedicated server, add the option OPTION(--disable-glout) to the configure script.
To change the installation path, use the option OPTION(--prefix=new_path).
Type CMDLINE([uninstall-PROGNAME-VERSION]) to uninstall.
])

PARAGRAPH([
Please ELINK(armagetron.sf.net/contact.html,report it) if this procedure gives you errors you cannot
resolve; the goal is to make it work on as many systems as possible.
])

SUBSECTION(Binary RPM,rpm)
PARAGRAPH([
Type CMDLINE(rpm -i filename.rpm) The RPM build is quite untested, so it may be
that it has registered too many dependencies and you are not allowed to install it; 
you may use the additional switch OPTION(--nodeps) to ignore these errors. However, you
may then end up with a broken installation.<br>
The documentation can then found in FILE(/usr/share/doc/PROGNAME).
To uninstall, use CMDLINE(rpm -e PROGNAME)
])

ifelse(,,,
SUBSECTION(Debian)
PARAGRAPH([
TODO
])
)

SUBSECTION(Source RPM)
PARAGRAPH([
Type CMDLINE(rpm --rebuild filename.src.rpm) 
This should build binary RPMs suitable for your system in FILE(/usr/src/redhat/RPMS/i386) (RedHat)
resp. FILE(/usr/src/packages/RPMS/i386) (SuSE); you may have to look into the other subdirectories
of FILE(RPMS) to find them. Proceed as described in the LINK([install_linux.html#rpm],[previous paragraph]).<br>
Alternatively, type CMDLINE(rpm -i filename.src.rpm)
This should extract a source package in FILE(/usr/src/.../SOURCES) you can handle like
described LINK([install_linux.html#source],[in the first section]). You can
surely ignore all failed dependencies with OPTION(--nodeps) since the 
dependencies were all there when you built the package. (It is possible,
if not likely, that the automatic dependency generation of RPM produces
junk.)
])

SUBSECTION(Non-Root install)
PARAGRAPH([
You have the option to install the game as a regular user in your home directory. To do so, use
the OPTION(--prefix) directive of OPTION(rpm) or the OPTION(configure) script to point to a 
place in your home directory, i.e.
CMDLINE(./configure --prefix /home/username/usr)
You can then start the game with
CMDLINE(/home/username/usr/bin/PROGNAME)
or, if you add OPTION(/home/username/usr/bin) to your executable search path, like normal.
])

SUBSECTION(Multiple versions,multiple)
If you give the additional option OPTION(--enable-multiver) to OPTION(configure), all installation
directories and executables will get the suffix OPTION(VERSION). Symbolic links will be created to
your convenience. This allows to install multiple versions in parallel, where OPTION(PROGNAME) will
always start the last installed version. dnl The RPMs are automatically built that way.

dnl SUBSECTION(netbsd)
dnl cd /usr/pkgsrc/games/armagetron
dnl make install

ifelse(,,,[
SUBSECTION(Binary archive)
PARAGRAPH([
Unpack the archive and change into the unpacked directory; type CMDLINE(./install)Everything will
be copied to FILE(PREFIX/games/armagetron). Type CMDLINE(PROGNAME --uninstall) to uninstall.
])
])

SECTION(Dedicated Server)
PARAGRAPH([
To install the dedicated server from binaries,
follow all the procedures above with the corresponding
download files; the server will be installed in
FILE(usr/local/games/PROGNAMEBASE-dedicated) and can be invoked by
CMDLINE(PROGNAMEBASE[-]dedicated)
Use the OPTION(--disable-glout) option of OPTION(configure) to build a dedicated server
when you are installing from source.
])

PARAGRAPH([
An init.d style start/stop script called FILE(PROGNAMEBASE) will be installed into FILE(/etc/init.d),
so you can use your standard system utilities to let it be started at boot time. By default, it runs
the server under the userid PROGNAMEBASE-dedicated (which gets created at installation) and logs
activity in FILE(/var/games/PROGNAMEBASE-dedicated). You can change these settings in the
configuration file FILE(CONFIGPATH/rc.config).
Unfortunately, the only way to give
commands to the server when it is run this way is to put them into FILE(everytime.cfg) which is 
read every round or through the ingame admin.
])

ifelse(,,,[
SECTION(Result)
define(DOCSTYLE_RESULT,unix)
include(install_result.html.m4)
]
)

SECTION(Play)
PARAGRAPH([
Type CMDLINE(PROGNAME)
as a normal user to play.
])

include(sig.m4)
include(navbar.html.m4)
</body>
</html>
