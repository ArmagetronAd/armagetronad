Compiling Armagetron Advanced on Windows with Code::Blocks
----------------------------------------------------------

Code::Blocks is an open source, cross platform C/C++ IDE.
It has multiple compiler support, and it comes in two presentations:
MinGW bundle or Standalone for use with other compilers.

More info and download of Code::Blocks: http://www.codeblocks.org

Armagetron Advanced can be compiled with these compilers:
* MinGw: obtained in the MinGW bundle download of Code::Blocks
* Free Microsoft Visual C++ Toolkit 2003: http://msdn.microsoft.com/visualc/vctoolkit2003/

The sources are distributed over three CVS modules:
* armagetronad: containing generic sources
* armagetronad_build_codeblocks: containing codeblocks project files
* winlibs: containing the libraries Armagetron Advanced depends on

You need to check out all three modules from the repository at
:ext:<your sf username>@armagetronad.cvs.sourceforge.net:/cvsroot/armagetronad
for developers or
:pserver:anonymous@armagetronad.cvs.sourceforge.net:/cvsroot/armagetronad
for everyone else.

1.  Create a project directory (e.g. C:\Projects\Armagetron Advanced)
2.  Put the armagetronad source files there
3.  In the same directory, put the armagetronad_build_codeblocks files
4.  In the same directory, put the winlibs files
5.  It should look something like this:
    +- Armagetron Advanced
       +- armagetronad
       +- armagetronad_build_codeblocks
       +- winlibs
6.  Go to the armagetronad_build_codeblocks directory
7.  Run the makedist.bat file to build the 'dist' and 'debug' directories
8.  Start the Armagetron Advanced workspace (Armagetron.workspace)
9.  To compile you will need to change the active project and build target 
    (it defaults to Armagetron Advanced Client and Win32 Release build target)

In case of problems, visit forums.armagetronad.net and ask for help.