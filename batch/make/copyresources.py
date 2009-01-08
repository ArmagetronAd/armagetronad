#!/usr/bin/env PYTHON

# This script replaces the previous sortresources shell script
# links resources from one directory to another and sorts them there
# usage: resources <source> <destination> <path to sortresources.py>

import sys, os
import shutil
import sortresources

def IgnoreFiles(visitdir, contents):
    ignoreList = ["CVS", ".svn", ".bzr"]

    for a in contents:
        if contents.endswith("~"):
            ignoreList.append(a)
            
    return ignoreList

def GetDtdVersion(dtdfile):
    dtdfileObj = open(dtdfile)

    for line in dtdfileObj:
        line = line.strip()
        
        if line.startswith("<!-- version="):
            varList = line.split('"')
            
            # the version should be the second item in the list
            dtdfileObj.close()

            return varList[1]
    return ""

def CopyDtd(source, destination, dtdVersion):
    sourceObj = open(source, "r")
    destObj = open(destination, "w")

    for line in sourceObj:
        line = line.replace("resource.dtd", dtdVersion)
        destObj.write(line)

    sourceObj.close()
    destObj.close()

if __name__ == "__main__":
    source = sys.argv[1]
    destination = sys.argv[2]
    #sortresources = sys.argv[3]
    print "Sorting resources from", source, "to", destination
    if os.path.exists(source) is False:
        sys.exit(1)

    print destination

    if os.path.exists(destination):
        shutil.rmtree(destination)

    shutil.copytree(source, destination, IgnoreFiles)

    print "Running sortresources"
    sortresources.main(["-v", destination] )
    print "Done"

    if os.path.exists(os.path.join(destination, "AATeam") ) is False:
        os.mkdir(os.path.join(destination, "AATeam") )

    resourceVersion = GetDtdVersion(os.path.join(source, "resource.dtd") )

    resourceName = "resource-" + resourceVersion + ".dtd"

    dtdList = os.listdir(source)

    for a in dtdList:
        base, ext = os.path.splitext(a)
        if ext.lower() == ".dtd":
            resourceVersion = GetDtdVersion(os.path.join(source, a) )
            print resourceVersion
            CopyDtd(os.path.join(source, a),
                    os.path.join(destination, "AATeam",
                                    base + "-" + resourceVersion + ".dtd"),
                    resourceName
                   )
            # a little hack to maintain compatibility with older sortresources shell script
            theTypeFile = open(os.path.join(destination, "." + base + "version"), "w" )
            theTypeFile.write(resourceVersion)
            theTypeFile.close()

