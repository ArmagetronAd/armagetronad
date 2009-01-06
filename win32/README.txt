Compiling Armagetron Advanced on Windows with Code::Blocks
----------------------------------------------------------

Code::Blocks is an open source, cross platform C/C++ IDE.
It has multiple compiler support, and it comes in two presentations:
MinGW bundle or Standalone for use with other compilers.

More info and download of Code::Blocks: http://www.codeblocks.org

Armagetron Advanced can be compiled with these compilers:
* MinGw: obtained in the MinGW bundle download of Code::Blocks
* Free Microsoft Visual C++ Toolkit 2003: http://msdn.microsoft.com/visualc/vctoolkit2003/

Currently I use the following nightly build of Code::Blocks:
	CB_20060428_rev2395_win32_wx263.7z
You also need an additional DLL:
	wxmsw26u_gcc_cb_wx2.6.3.7z

Download Link: http://developer.berlios.de/project/showfiles.php?group_id=5358

The sources are distributed over two SVN modules:
* armagetronad: containing generic sources and codeblocks project files
* winlibs: containing the libraries Armagetron Advanced depends on

You need to check out the two modules from the repository at

https://armagetronad.svn.sourceforge.net/svnroot/armagetronad

You can just check out
https://armagetronad.svn.sourceforge.net/svnroot/armagetronad/trunk, that
way you'll have the correct directory structure, although you'll also get
some files you don't need. One way to do that is to use TortoiseSVN
(http://tortoisesvn.tigris.org/).

1.  Create a project directory (e.g. C:\Projects\Armagetron)
2.  put the armagetronad source files there
3.  In the same directory, put the winlibs files
4.  It should look something like this:
    +- Armagetron
       +- armagetronad
       +- winlibs
5.  Check you've got python installed and set the path to the executable
    inside python.bat
6.  Go to the armagetronad/win32 directory
    run update_version.bat
7.  Start the Armagetron workspace (Armagetron.workspace)
8.  To compile you will need to change the project's target to either
    Release, Debug or Profile (it defaults to Release)
9.  Go to the armagetronad/win32 directory
    Run the makedist.bat file to copy all neccessary files into
    the build directories (dist, debug or profile).
    +- Armagetron
       +- armagetronad
          +- build
             +- dist
             +- debug
             +- profile

Note: Only directories that are already present will be updated.
      At the end you will get a status report about missing files.

To create an installer you need to install the nullsoft installer development
environment (http://nsis.sourceforge.net/). After that you can right click
on armagetronad.nsi in the appropriate build directory (not in the win32
directory, it'll fail) and select "Execute NSIS script".

In case of problems, visit forums.armagetronad.net and ask for help.
