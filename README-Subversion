In this file you find instructions how to get from a fresh Subversion checkout to a running game.
It is assumed that you know how to check out modules on your system, because you apparently already managed to check out one :) "Checking out module X in parallel to this" means you should go to the parent directory and check out module X from the same repository where you got this module from, so that the parent directory looks like this:

parent_directory:
    armagetronad:
        README-Subversion and other files
    X:
        files from X

To use the sources from Subversion, you need to have additional tools installed: on all operating systems, you'll need Python (www.python.org). On Unix, you also need all autotools (autoconf and automake, check with your distribution).
To build tarball distributions, you also need a 'svn' command line tool that can generate ChangeLogs.

****************
* Unix vanilla *
****************
You're in luck, you already have everything you'll need. Follow these steps:

# generate configure, config.h.in and the like
> sh ./bootstrap.sh
# generate and change to build directory
> mkdir ../armagetronad-build && cd ../armagetronad-build
# run the configure script ( add --disable-glout for the dedicated server )
> ../armagetronad/configure
# run make
> make
# run game
> make run
# or run with arguments
> ./src/armagetronad_main <options>
# install game if you like
> make install

****************
* Unix managed *
****************

There is also a helper module for the unix build, managing the tasks above and managing debug and optimized versions of the client and the server. It is designed to operate with the Eclipse IDE, but runs fine without it. The module's name is armagetronad_build_eclipse.
Check it out parallel to the armagetronad module, cd there and read the enclosed README.
If you're in a hurry, just run "make" there.

****************
* KDevelop 3.0 *
****************

First, you need to bootstrap. Execute: sh ./bootstrap.sh
Run 'kdevelop' if you haven't done so already.
From the Project menu, choose Open Project
Select 'armagetron.kdevelop' and press OK
From the Build menu, choose Run Configure

If all goes smoothly, you can now edit Armagetron Advanced and run it.

********
* OS X *
********

## Prerequisites

* [Apple developer tools](http://developer.apple.com)
    * Xcode 2.2 or greater is required.
* [rake](http://rubyforge.org/projects/rake)
* [Other Libraries](http://generalconsumption.org/armagetron/MacOSX_AALibraries.zip)
    * Unzip and install to /Libraries

## Building

1. Open “MacOS/Armagetron Advanced.xcodeproj”.
2. There is a big “Build” button. Click it.
3. An “armagetronad-build” directory will be created along-side the source directory, in which generated files and products are stored.

*********************
* Windows + VisualC *
*********************

You'll need to check out the additional module build_visualc parallel to this module. In it, you'll find a README describing how to proceed; you'll need to download some libraries and put them in the right place.

********************
* Windows + Cygwin *
********************

Not supported yet.

*******************
* Windows + Mingw *
*******************

Not supported yet.



------------------------------------------
Initialy written by z-man.
Some changes, including Subversion migration, by Luke-Jr
Original KDevelop 3.0 instructions by subby
