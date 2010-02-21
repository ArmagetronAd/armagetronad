#!/usr/bin/python

# This script replaces the previous sortresources shell script
# links resources from one directory to another and sorts them there
# usage: resources <source> <destination> <path to sortresources.py>

import sys, os
import shutil
foundModule = False

newPathSearch = os.path.dirname(os.path.abspath(sys.argv[0]) )
sys.path.insert(0, newPathSearch)

maxnumAttempts = 10
numAttempts = 0

while not foundModule:
    try:
        import armabuild
        foundModule = True
        #print "Found armabuild!"
    except:
        sys.path[0] = os.path.dirname(newPathSearch)
    numAttempts += 1

    if numAttempts > 9:
        print "Unable to find armabuild module.  Can't continue."
        sys.exit(1)

from armabuild import resource

def IgnoreFiles(visitdir, contents):
    ignoreList = ["CVS", ".svn", ".bzr"]

    for a in contents:
        if contents.endswith("~"):
            ignoreList.append(a)
            
    return ignoreList

if __name__ == "__main__":
    source = sys.argv[1]
    destination = sys.argv[2]
    #sortresources = sys.argv[3]
#    print "Sorting resources from", source, "to", destination
    if os.path.exists(source) is False:
        sys.exit(1)

    if os.path.exists(destination):
        shutil.rmtree(destination)

    shutil.copytree(source, destination, IgnoreFiles)

#    print "Running sortresources"
    resource.main([destination] )
#    print "Done"

    if os.path.exists(os.path.join(destination, "AATeam") ) is False:
        os.mkdir(os.path.join(destination, "AATeam") )

    resourceDtd = os.path.join(source, "resource.dtd")
    if os.path.exists(resourceDtd) is False:
        resourceDtd = os.path.join(os.path.join(source, "proto"), "resource.dtd")
    resourceVersion = resource.GetDtdVersion(resourceDtd)

    resourceName = "resource-" + resourceVersion + ".dtd"

    dtdList = os.listdir(source)

    for a in dtdList:
        base, ext = os.path.splitext(a)
        if ext.lower() == ".dtd":
            resourceVersion = resource.GetDtdVersion(os.path.join(source, a) )
            
            resource.CopyDtd(os.path.join(source, a),
                    os.path.join(destination, "AATeam",
                                    base + "-" + resourceVersion + ".dtd"),
                    resourceName
                   )
            # a little hack to maintain compatibility with older sortresources shell script
            theTypeFile = open(os.path.join(destination, "." + base + "version"), "w" )
            theTypeFile.write(resourceVersion)
            theTypeFile.close()

