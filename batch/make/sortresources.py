#!/usr/bin/python
# sorts resources by their included name and version information
# usage: call from the directory containing the resources or
# call
# sortresources.py <path_to_seach_and_sort>
# use sortresources.py -h to get command line option help

from xml import sax
from xml.sax import handler
import string
import os.path
import os
import sys
import StringIO

# parse a resource xml file and fill in data property data strucure
def parseResource(filename, data):
    # document parser class
    class docHandler(handler.ContentHandler):
        def __init__(self,data):
            self.data = data

            # fill in defaults
            data.type = "unknown"
            data.name = ""
            data.version = "1.0"
            data.author = "Anonymous"
            data.category = ""

        def startElement(self,name,attr):
            # just parse the <map> tag and fill data
            if name in("Resource","Map","Cockpit"):
                # set type from tag name it is not already set
                try: oldtype = self.data.type
                except: self.data.type = name.lower()       

                # read values from file    
                if "name" in attr.getNames():
                    self.data.name     = attr.getValue("name")
                else:
                    return #invalid element
                if "author" in attr.getNames():
                    self.data.author   = attr.getValue("author")
                else:
                    return #invalid element

                if "type" in attr.getNames():
                    self.data.type     = attr.getValue("type")
                if "version" in attr.getNames():
                    self.data.version  = attr.getValue("version")
                if "category" in attr.getNames():
                    self.data.category = attr.getValue("category")
                    
    # rudimentary entity resolver
    class entityResolver:
          # initialize, storing a path and a data object
          def __init__(self,path,data):
              self.path = path
              self.data = data

          # return a stream for a given external entity
          def resolveEntity(self,pubid,sysid):
              path = self.path

              # extract data type: the base name of the DTD
              self.data.type = string.split(sysid,"-")[0]

              # look in current and parent directories for dtds
              while len(path) > 0 and path != "/":
                  fullfile = os.path.join(path, sysid)
                  try:
                      return open( fullfile )
                  except:
                      pass
                  # go to "parent" directory
                  path = os.path.split(path)[0]

              # fallback: return empty stream, result: no dtd checking is done.
              # Not horribly bad in this context
              print "warning, could not find requested entity", sysid
              
              return StringIO.StringIO("")

    # parse: create parser...
    parser = sax.make_parser()
    # set it's content handler to extract the category data we want...
    parser.setContentHandler( docHandler(data) )
    # give it a dummy entity resolver that tries to find external entites in a
    # half-assed way...
    parser.setEntityResolver( entityResolver(os.path.split(filename)[0], data) )
    # and go!
    parser.parse( open(filename) )
    
    # exception rule for the moment: Translate type = map to type = aamap.
    try: 
         if data.type == "map":
            data.type = "aamap"
         if data.type == "cockpit":
            data.type = "aacockpit"
    except: pass

# take the parsed data and trandsform it into a filename
def getName(data):
    return os.path.join(data.author, data.category, data.name) + "-" + data.version + "." + data.type + ".xml"

# determine the canonical name of a xml resource
def getCanonicalPath(path):
    # trivial data class
    class Data:
        pass

    # parse xml file and return name
    data = Data()
    parseResource(path, data)
    return getName(data)

# scan for all XML files in the directory and rename them according to the rules
def scanDir(sourceDir, destinationDir, function):
    def visitor( arg, dirname, names ):
        for filename in names:
            if filename.endswith(".xml"):
                path = os.path.join(dirname, filename)
                newPath = getCanonicalPath(path)
                # call the passed function
                function(path, os.path.join(destinationDir, newPath), newPath)
        
    os.path.walk(sourceDir, visitor, visitor )

# move file oldFile to newFile
def Move(oldFile, newFile, canonicalPath ):
    if oldFile == newFile:
        return False

    Print( oldFile, newFile, canonicalPath )

    # generate target directory
    dir = os.path.split(newFile)[0]
    if not os.path.exists(dir):
        os.makedirs(dir)

    # move file
    os.rename(oldFile, newFile)
    return True

# dummy replacement: just print
def Print(oldFile, newFile, canonicalPath ):
    if oldFile == newFile:
        return False

    if doPrint:
        print "renaming", oldFile, "->", newFile

# move file oldFile to newFile, setting apache rewrite rules to keep the file
# fetchable from its old position
def Redirect(oldFile, newFile, canonicalPath ):
    # move the file
    if Move( oldFile, newFile, canonicalPath ):
        # determine full path
        fullCanonicalPath = "/resource/" + canonicalPath

        # file was really moved, see what directory it came from
        oldPath = os.path.split( oldFile )

        # determine filename of the .htaccess file
        htaccessPath = os.path.join( oldPath[0], ".htaccess" )

        # determine whether we need to add the RewriteEngine On line:
        needEngine = True
        try:
            # open .htaccess file in the old directory for reading
            accessFile = open(htaccessPath)
            for line in accessFile:
                splitLineLower = string.split(line.lower())
                splitLine      = string.split(line)
                if splitLineLower[0] == "rewriteengine":
                    # rewriting is already mentioned; we don't have to enable it laster.
                    needEngine = False

                # see if there already is a rewrite rule for our file
                if splitLineLower[0] == "rewriterule" and splitLine[1] == oldPath[1]:
                    if splitLine[2] == fullCanonicalPath:
                        # nothing to do
                        return
                    else:
                        print "Warning: There already is a different RewriteRule for", oldFile, "in place."
        except: pass

        # open .htaccess file for appended writing
        accessFile = open(htaccessPath, "a")
        
        # enable rewriting
        if needEngine:
            accessFile.write("RewriteEngine On\n")

        # add rewrite rule
        accessFile.write( "RewriteRule " + oldPath[1] + " " + fullCanonicalPath + "\n");

# print usage message and exit.
def Options( ret ):
    print
    print "Usage: sortresources.py [OPTIONS] <source directory> <destination directory>"
    print "       Omitting the destination directory makes it equal to the source."
    print "       Omitting both directories makes them the current working directory."
    print "Options: -r add apache rewrite rules"
    print "         -n don't do anything, just print what would be done"
    print "         -v print non-warning status messages"
    print "         -h print this message"
    print
    sys.exit( ret )

def main(argList):
    destinationDirectory = "."
    sourceDirectory = "."
    function  = Move

    global doPrint
    doPrint = False

    # parse arguments
    for arg in argList:
        # parse options
        if len(arg)>0 and arg[0] == "-":
            if len(arg) > 1:
                if arg[1] == "h":
                    Options( 0 )
                    continue
                if arg[1] == "n":
                    function = Print
                    doPrint = True
                    continue
                if arg[1] == "r":
                    function = Redirect
                    continue
                if arg[1] == "v":
                    doPrint = True
                    continue
                print "\nUnknown option:", arg
                Options(-1)
            else:
                Options(-1)
        else:
            # non-dashed argument gives the target and destination directory
            if "." == destinationDirectory:
                sourceDirectory = arg
            destinationDirectory = arg
    
    # print sourceDirectory, destinationDirectory

    # do the work
    scanDir(sourceDirectory, destinationDirectory, function)
    
    
if __name__ == "__main__":
    main(sys.argv)
