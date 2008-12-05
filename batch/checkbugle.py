#!/usr/bin/python

# checks bugle trace log for OpenGL problems

import sys

count = 0
lineNo = 0
inList = False
inBlock = False
legalLists = {}
setLists = {}
usedInList = {}
usedInBlock = {}
usedOutBlock = {}

for line in sys.stdin:
    line=line[line.find('gl'):]

    # split line into functional parts
    op=line.find('(')

    # the function mane
    function=line[0:op]
    rest=line[op+1:-1]
    cl=rest.find(')')

    # the argument list
    args=rest[0:cl]
    rest=rest[cl+1:]

    # the result
    result=''
    eq=rest.find('= ')
    if eq >= 0:
        result = rest[eq+2:]

    lineNo=lineNo+1
    
    if False and function.find( 'List' ) >= 0 and function.find( 'Call' ) < 0:
        print "%d (%s) (%s) (%s) (%s)" % (count, line[:-1], function, args, result)

        count = count + 1
        if count > 100:
            exit(-1)

    if function == 'glBegin':
        if inBlock:
            print "Error: Still in block."
            exit(-1)
        inBlock = True
    elif function == 'glEnd':
        if not inBlock:
            print "Error: Not in block."
            exit(-1)
        inBlock = False
    else:
        blockDict=usedOutBlock
        if inBlock:
            blockDict=usedInBlock
        if not function in blockDict:
            blockDict[function]=lineNo

    if function == 'glGenLists':
        legalLists[result] = True
        if inList:
            print "Error: Still in list generation."
            exit(-1)

    if function == 'glEndList':
        if not inList:
            print "Error: Not in list generation."
            exit(-1)
        inList=False

    if function == 'glNewList':
        list=args[0:args.find(',')]
        if inList:
            print "Error: Still in list generation."
            exit(-1)
        if not legalLists[list]:
            print "Error: list %s used, but not generated." % [list]
            exit(-1)
        setLists[list]=True
        inList=True
    elif inList:
        if not function in usedInList:
            usedInList[function]=lineNo
            #print lineNo, function
        
    if function == 'glCallList':
        list=args
        if not legalLists[list]:
            print "Error: list %s used, but not generated." % [list]
            exit(-1)
        if not setLists[list]:
            print "Error: list %s used, but not set." % [list]
            exit(-1)

    if function == 'glDeleteLists':
        list=args[0:args.find(',')]
        if not legalLists[list]:
            print "Error: list %s used, but not generated." % [list]
            exit(-1)
        legalLists[list]=False
        setLists[list]=False

print "Used in display lists:"
for f in usedInList:
    print f, usedInList[f]

print
print "Used in glBegin/End:"
for f in usedInBlock:
    print f, usedInBlock[f]

print
print "Used outside glBegin/End:"
for f in usedOutBlock:
    print f, usedOutBlock[f]
    
    
