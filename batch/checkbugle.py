#!/usr/bin/python

"""checks bugle trace log for OpenGL problems"""

from __future__ import print_function
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

def error(error, lineNo, *args):
    print("Error:", error.format(*args), lineNo, file=sys.stderr)
    exit(-1)

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
        print(" ".join((count, line[:-1], function, args, result)))

        count = count + 1
        if count > 100:
            exit(-1)

    if function == 'glBegin':
        if inBlock:
            print("Error: Still in block.", lineNo)
            exit(-1)
        inBlock = True
    elif function == 'glEnd':
        if not inBlock:
            print("Error: Not in block.", lineNo)
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
            error("Still in list generation.", lineNo)

    if function == 'glEndList':
        if not inList:
            error("Not in list generation.", lineNo)
        if inBlockAtListStart != inBlock:
            error("glBegin/glEnd mismatch in list.", lineNo)
        inList=False

    if function == 'glNewList':
        inBlockAtListStart=inBlock
        l=args[0:args.find(',')]
        currentList=l
        if inList:
            error("Still in list generation.", lineNo)
        if not legalLists[l]:
            error("list {} used, but not generated.", lineNo, l)
        setLists[l]=True
        inList=True
    elif inList:
        if not function in usedInList:
            usedInList[function]=lineNo
            #print lineNo, function
        
    if function == 'glCallList':
        l=args
        if not legalLists[l]:
            error("list {} used, but not generated.", lineNo, l)
        if inList and currentList == l:
            error("list {} used, but it's just getting generated.", lineNo, l)
        if not setLists[l]:
            error("list {} used, but not set.", lineNo, l)

    if function == 'glDeleteLists':
        l=args[0:args.find(',')]
        if not legalLists[l]:
            error("list {} used, but not generated.", lineNo, l)
        legalLists[l]=False
        setLists[l]=False

print("Used in display lists:")
for f in usedInList:
    print(f, usedInList[f])

print()
print("Used in glBegin/End:")
for f in usedInBlock:
    print(f, usedInBlock[f])

print()
print("Used outside glBegin/End:")
for f in usedOutBlock:
    print(f, usedOutBlock[f])
    
    
