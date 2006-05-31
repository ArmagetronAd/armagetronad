This file is intended as a guide to developers. It is a constant work in progress.

If you want to become a developer, you should register at the Armagetron Advanced Forum at
http://forums.armagetronad.net/viewtopic.php?t=1360
Just tell us that you want to join and what your skills are. We can always use another hand.

The information here is for the Unix side of development, Windows developers just need to read README-Subversion to get going.

**********************************************************
* Flags and Settings only developers need to know about: *
**********************************************************

The general rule of developing solid software is that as many bugs as possible are found during development. To faciliate this, the configure script understands some options for developers. They are triggered by giving the shell variables DEBUGLEVEL and CODELEVEL to numeric values. The tables below defines the possible settings. Every setting includes all the effects of the previous settings.

DEBUGLEVEL: determines the level of debugging checks
0   No debugging. If no further CXXFLAG is set, this enables the -Os optimization flag.
1   Disables optimizations and enables debugging symbols.
2   Enables tASSERT assertions and other things that do not eat away too much CPU time.
3   Enables the leak-finding memory manager.
4   Enables extensive consistency checks. This slows down the system considerably, but it is still playable.
5   Enables full memory management debugging. Slows down things to a crawl.

CODELEVEL: determines the level of compiler checks for sensible code
0   No checks.
1   Enables -Wall compiler flag.
2   Enables various other warnings that are not too restrictive.
3   Enables all warnings that sound good in theory.
4   Enables -Werror.

z-man feels that developers should use at least DEBUGLEVEL=3 in daily operation. Higher debug levels should be used when bugs are hard to find otherwise. Likewise, CODELEVEL=2 should be the default for developers and the code should compile without warnings. (Hint: also setting "CXXFLAGS=-Werror" helps not to miss any warning by mistake) Warning options activated by CODELEVEL=2 that turn out to be too restrictive should be reported to the forum, they are negotiable.

**********************************************************
* How to run and debug:                                  *
**********************************************************

Use "make debug" to create debugging files: a symbolic link to the executable from the build directory and .gdbinit setting a default breakpoint. 
You can then use "gdb armagetronad(-dedicated)" to start the game in the debugger.
Use "make run" to run the game from within the build directory; it should automatically gather its data files from the source and build directory. 
In this mode of operation, variable files are stored inside the build directory in the var subfolder. 

**********************************************************
* Source code reformatting:                              *
**********************************************************

There exist three source code reformatting targets for different needs:
"make rebeautify" is for developers who are happy with the project code formatting settings. It enforces them and touches the timestamps of changed files.
"make beatuify-personal" and "make beautify" apply your personal formatting rules (from ~/.astylerc) and the project defaults without updating the timestamps for those who want to switch between two styles without full rebuilds in between.

**********************************************************
* Checks:                                                *
**********************************************************

Build system integrity checks:
"make installcheck" installs the game in a local directory tree (._inst), tests its basic operation and uninstalls it, checking whether there are leftover files.
"make devcheck" runs beautification and "make installcheck".
"make distcheck" (provided by automake) builds a distributable source tarball and checks whether "make (dist)clean" and various installations work correctly. It makes a full rebuild and is time consuming.
"make fullcheck" runs all of the above.
z-man's advice: Before committing your work to Subversion, please run at least "make devcheck", it only takes some seconds. Run "make fullcheck" when you have made changes to the build system.

-------------------------------------------------------
initially written by z-man
