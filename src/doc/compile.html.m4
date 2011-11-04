include(head.html.m4)
define(SECT,compile)
include(navbar.html.m4)


TITLE(Redistribution)

PARAGRAPH([
This page is intended to give GNU/Linux distribution builders some help in building packages
of PROGTITLE for their system. For the average user, it is of little use.
])

PARAGRAPH([
The most important remark: this game is distributed under the GNU general public license.
You can redistribute it in any way you want as long as you don't make changes: Put in on 
cover CDs of magazines (we are happy to receive a copy of the corresponding edition, but it's
not mandatory), add it to your freeware download directory or even put it in a box and sell it if
you're crazy enough. You don't have to ask us for permission for any of this.<br>
The GNU license kicks in if you make changes. Then, you also have to publish the sources
of these changes.
])

PARAGRAPH([
In theory, you always have to make the sourcecode available when
you redistribute. However, this documetnation contains links to the official
download locations, and they count as "making the source available" in our eyes.
])

SECTION(Packaging)

PARAGRAPH([
We manage distribution building with an additional module we don't put into regular releases;
it is called FILE(build) and also hosted at Launchpad. Get it with
CMDLINE(bzr branch http://bazaar.launchpad.net/%7Earmagetronad-dev/armagetronad/0.2.8-build-work/)
enter it and type FILE(make) once to configure it to your system. Edit FILE(make.conf) to put
in the missing details. Then, drop a tarball from our distribution into the FILE(tarballs)
subdirectory and use FILE(make) to build the package of your choice. See the enclosed README.txt
to see what's available.
])

SECTION(Roll your own)
PARAGRAPH([
If you want to roll your own package distribution, there are two special switches to FILE(configure)
you'll need to know about:
])
PARAGRAPH([
The first is OPTION(--disable-sysinstall). Normally, OPTION(make install) copies the files
to their destination and then calls the script FILE(batch/sysinstall), telling it where the
files were put (just the prefix, i.e. FILE(/usr/local)), and the script then finishes off the
installation. It adapts the server starter scripts to the real location, adds symbolic links to
FILE(/etc/PROGNAME) and FILE(/etc/init.d/) for the server, and it creates the user the server
should run as. Naturally, if you want to build a binary distribution package, you want this
script to run on the user's machine, not yours. Therefore, you add the option
OPTION(--disable-sysinstall) to the invocation of FILE(configure) which will prevent the script
from being run. Then, you should set up the installation package so that it calls
CMDLINE({prefix}/share/PROGNAME/scripts/sysinstall install {prefix})
after the regular files have been installed. {prefix} stands for the user choosable installation
prefix (PROGTITLE is relocatable). Before uninstallation, you call
CMDLINE({prefix}/share/PROGNAME/scripts/sysinstall uninstall {prefix})
])
PARAGRAPH([
The second option is OPTION(--enable-multiver). Enable it if you want to allow users to install
multiple versions of you package concurrently. All directories and executables will get the
version number as suffix so there are no file collisions, and the sysinstall script installs
additional links to the executables for the user's convenience.
])

include(sig.m4)
include(navbar.html.m4)
</body>
</html>
