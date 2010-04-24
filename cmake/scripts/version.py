#!/usr/bin/python

from optparse import OptionParser
optionParser = OptionParser("Usage: %prog [options] SOURCE_DIR")
optionParser.add_option("-o", "--output", dest="outputFileName", help="Write output to this file", metavar="FILE")
optionParser.add_option("-v", "--var", dest="var", help="Only output this variable", metavar="VAR")
optionParser.add_option("",   "--release", dest="release", default=False, help="Configure version number for a release", metavar="VERSION")

(options, args) = optionParser.parse_args()

if len(args) != 1:
    optionParser.error("Incorrect number of arguments.")

from bzrlib.workingtree import WorkingTree
from bzrlib.branch import Branch
from time import gmtime, strftime

date = strftime("%Y%m%d")

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
        'BRANCHURL': "Unknown"
}

# Open the branch and the working tree and start fetching info
branch = Branch.open( args[0] )
tree = WorkingTree.open( args[0] )
graph = branch.repository.get_graph()

# Let's start with the easy stuff!
outvars['REVID'] = tree.last_revision()
outvars['REVNO'] = branch.revision_id_to_revno( outvars['REVID'] )
outvars['BRANCHNICK'] = branch.nick

# Build a function in order to calculate a revision's znumber(count of revisions prior and including it)
# We assume here we'll be computing ZNRs of late revisions rather than early ones
revs = branch.repository.get_ancestry( outvars['REVID'] )
revs.reverse()
numrevs = len(revs)
def ZNR( revid ):
    i = 0
    for rev in revs:
        if revid == rev:
            return numrevs - i
        else:
            ++i

outvars['ZNR'] = ZNR( outvars['REVID'] )

if outvars['REVID'] in branch.tags.get_reverse_tag_dict():
    outvars['REVTAG'] = branch.tags.get_reverse_tag_dict()[ outvars['REVID'] ].pop()

# Determine public URL
if branch.get_public_branch():
    outvars['BRANCHURL'] = branch.get_public_branch()
elif branch.get_bound_location():
    outvars['BRANCHURL'] = branch.get_bound_location()
elif branch.get_push_location():
    outvars['BRANCHURL'] = branch.get_push_location()
elif branch.get_old_bound_location():
    outvars['BRANCHURL'] = branch.get_old_bound_location()

# Open parent branch and find last common ancestor.
parentBranch = Branch.open( branch.get_parent() )
branch.lock_read()

lca = graph.find_lca( outvars['REVID'], parentBranch.last_revision() ).pop()
outvars['BRANCHLCA'] = parentBranch.revision_id_to_revno( lca )
outvars['BRANCHLCAZ'] = ZNR( lca )

branch.unlock()

# Find out what the full version number tag looks like.
if options.release:
    outvars['VERSION'] = options.release
else:
    # mainline branch
    outvars['VERSION'] += outvars['BRANCHNICK'].split('-', 1)[0]
    # last common ancestor to the mainline branch
    outvars['VERSION'] += "_alpha_z%(znr)i" % { 'znr': outvars['BRANCHLCA'] }

    # changed from mainline?
    if lca != outvars['REVID'] or tree.changes_from( tree.basis_tree() ):
        outvars['VERSION'] += "_" + date
    # tagged ?
    elif outvars['REVTAG']:
        outvars['VERSION'] += "_" + outvars['REVTAG']


out = str()
#iter = outvars.iteritems()
for var, val in outvars.iteritems():
    # Convert python types to C++ types, and put quotes on strings
    if type(val) == str or type(val) == unicode:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s \"%(val)s\"" %{ 'var': var, 'val': val }])
    elif type(val) == bool and val == True:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s true" %{ 'var': var, 'val': val }])        
    elif type(val) == bool and val == False:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s false" %{ 'var': var, 'val': val }])        
    else:
        out = '\n'.join([out, "#define TRUE_ARMAGETRONAD_%(var)s %(val)s" %{ 'var': var, 'val': val }])

if options.outputFileName:
    f = open(options.outputFileName, "w")
    f.write(out.join('\n'))
else:
    print out

