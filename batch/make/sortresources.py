#!/usr/bin/python3
# sorts resources by their included name and version information
# usage: call from the directory containing the resources or
# call
# sortresources.py <path_to_seach_and_sort>
# use sortresources.py -h to get command line option help

import sys, os

foundModule = False

newPathSearch = os.path.dirname(os.path.abspath(sys.argv[0]) )
sys.path.insert(0, newPathSearch)

maxnumAttempts = 10
numAttempts = 0

while not foundModule:
    try:
        import armabuild
        import armabuild.resource
        foundModule = True
        print("Found armabuild!")
    except:
        sys.path[0] = os.path.dirname(newPathSearch)
    numAttempts += 1

    if numAttempts > 9:
        print("Unable to find armabuild module.  Can't continue.")
        sys.exit(1)

if __name__ == "__main__":
    armabuild.resource.main(sys.argv[1:])
