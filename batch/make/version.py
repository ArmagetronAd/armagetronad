#!/usr/bin/python

from optparse import OptionParser
optionParser = OptionParser()
optionParser.add_option("-o", "--output", dest="outputFileName", help="Write output to this file", metavar="FILE")
optionParser.add_option("-v", "--var", dest="var", help="Only output this variable", metavar="VAR")

(options, args) = optionParser.parse_args()

import bzrlib
from time import gmtime, strftime

outvars = {
        'VERSION': "",
        'REVID': "",
        'REVNO': 0,
        'BRANCHNICK': "",
        'ZNR': 0,
        'REVTAG': "",
        'CHANGED': True,
        'BUILDDATE': strftime("%a, %d %b %Y %H:%M:%S +0000", gmtime()),
        'BRANCHLCA': 0,
        'BRANCHLCAZ': 0,
        'BRANCHURL': ""
}

out = str()
#iter = outvars.iteritems()
for var, val in outvars.iteritems():
    # Convert python types to C++ types, and put quotes on strings
    if type(val) == str:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s \"%(val)s\"" %{ 'var': var, 'val': val }])
    elif type(val) == bool and val == True:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s true" %{ 'var': var, 'val': val }])        
    elif type(val) == bool and val == False:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s false" %{ 'var': var, 'val': val }])        
    else:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s %(val)s" %{ 'var': var, 'val': val }])

if options.outputFileName:
    with open(options.outputFileName, "w") as f:
        f.write(out)
else:
    print out

