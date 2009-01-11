#!/usr/bin/env python

# This script copies everything needed to build a windows distribution
# to the appropriate places.
# If run without commandline options, this script assumes it's running
# from batch/win32

import os, glob, shutil, sys, stat

foundModule = False

newPathSearch = os.path.dirname(os.path.abspath(sys.argv[0]) )
sys.path.insert(0, newPathSearch)

maxnumAttempts = 10
numAttempts = 0

while not foundModule:
    try:
        import armabuild
        foundModule = True
        print "Found armabuild!"
    except:
        sys.path[0] = os.path.dirname(newPathSearch)
    numAttempts += 1

    if numAttempts > 9:
        print "Unable to find armabuild module.  Can't continue."
        sys.exit(1)
    

# List of library directories that contain binary libaries
# Please make sure to use os.path.join and don't assume this script
# is only ever going to run in Windows....
winlibDirs = [os.path.join('SDL_image', 'VisualC','graphics','lib'),
               os.path.join('SDL_mixer','VisualC','smpeg','lib'),
               os.path.join('libxml2','lib'),
               os.path.join('iconv','lib'),
               os.path.join('FTGL','win32','freetype','lib') ]

# List of directories that are needed in the dist directory unmodified
armaDirs = [
            "config",
            "language",
            "models",
            "music",
            "sound",
            "textures"]

# List of files in arma root that are also needed in the dist directory
# unmodified.  Wildcards are fine.
armaFiles = ["*.txt",
             "README",
             "README-SDL" ]

# List of files used by the nsis installer.  Wildcards fine here, too.
installerFiles = ["*.nsi",
                  "*.bmp",
                  "*.url" ]

def IgnoreFiles(visitdir, contents):
    ignoreList = ["CVS", ".svn", ".bzr"]

    for a in contents:
        if a.endswith("~"):
            ignoreList.append(a)
            
    return ignoreList

def CopyTree(source, destination):
    if os.path.exists(source):
        shutil.copytree(source, destination, IgnoreFiles)
    # None of the code down there works for me, but it should
    '''
    print source, destination
    if os.path.exists(source):
        if(source.endswith("/") or source.endswith("\\") ):
            source = source[0:len(source)-1]
        basesource = os.path.dirname(source)
        
        if not os.path.exists(destination):
            os.mkdir(destination)
        for root, dirs, files in os.walk(source):
            aPath = root.replace(basesource, "")
            destPath = aPath
                
            for a in IgnoreFiles(os.path.join(source, root), dirs + files):
                if a in dirs:
                    dirs.remove(a)
                if a in files:
                    files.remove(a)
            for a in dirs:
                if not os.path.exists(os.path.join(destination, a) ):
                    os.mkdir(os.path.join(destination, a) )
                    os.chmod(os.path.join(destination, a),
                        stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |
                        stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP |
                        stat.S_IROTH | stat.S_IXOTH )
            for a in files:
                print destination
                CopyFile(os.path.join(root, a),
                         os.path.join(destination, destPath, a) )
                         '''

# Only copies the file if it doesn't exist, or if the source was
# changed.  Uses timestamp to determine changes.
def CopyFile(source, destination):
    if os.path.exists(destination):
        if os.path.getmtime(source) > os.path.getmtime(destination):
            shutil.copy2(source, destination)
    else:
        shutil.copy2(source, destination)

if __name__ == "__main__":
    from optparse import OptionParser

    parser = OptionParser()

    parser.add_option("-s", "--source-dir", action="store",
            default=os.path.join("..", ".."),
            help="location of armagetronad sources", dest="sources")
    parser.add_option("-b", "--build-dir", action="store",
            default=os.path.join("..", "..", "build"),
            help="location of build directory", dest="builddir")
    parser.add_option("-l", "--libs", action="store",
            default=os.path.join("..", "..", "..", "winlibs"),
            help="location of winlibs module", dest="winlibsdir")
    (options, args) = parser.parse_args()

    if not os.path.exists(options.builddir):
        os.mkdir(options.builddir)

    for a in ['dist', 'debug', 'profile']:
        destdir = os.path.join(options.builddir, a)
        if not os.path.exists(os.path.join(options.builddir, a) ):
            os.mkdir(destdir)

        # Copy whole directories first
        for b in armaDirs:
            CopyTree(os.path.join(options.sources, b), os.path.join(destdir, b) )
            sys.exit()

        # Copy the files now
        for b in armaFiles:
            for c in glob.glob(os.path.join(options.sources, b) ):
                CopyFile(c, destdir)

        winlibSourceDirs = winlibDirs
        winlibSourceDirs.append("build")
        
        for b in winlibSourceDirs:
            for c in glob.glob(os.path.join(options.winlibsdir, b, "*.dll") ):
                CopyFile(c, destdir)

        for b in installerFiles:
            for c in glob.glob(os.path.join(options.sources, "win32", b) ):
                CopyFile(c, destdir)

        # todo: add sortresources stuff here


