#!/bin/bash

# prepares debian sources
# parameters: <series> (unstable/karmic/gutsy...)

# import gpg keys
EXIT=0
gpg --import pub.gpg 2>&1 | tee importlog || EXIT=$?
gpg --allow-secret-key-import --import sec.gpg || EXIT=$?
rm -rf *.gpg
test ${EXIT} = 0 || exit ${EXIT}

set -x

# determine email of key owner
#grep "public key .* imported" importlog
#grep "public key .* imported" importlog | sed -e 's/>".*$/>/' -e 's/^.*"//' 
MAIL=`grep "public key .* imported" importlog | sed -e 's/>".*$/>/' -e 's/^.*"//'`

SERIES=$1
test -z ${SERIES} && SERIES=unstable

. version.sh || exit $?

DEBIAN_VERSION=`echo ${PACKAGE_VERSION} | sed -e s,_,~,g`~ppa1~${SERIES}

mv debian/changelog changelog_orig || exit $?
cat > debian/changelog <<EOF
armagetronad (${DEBIAN_VERSION}) ${SERIES}; urgency=medium

  * Updated to new upstream version

 -- ${MAIL}  `date -R`

EOF
cat changelog_orig >> debian/changelog || exit $?

. `dirname $0`/rebrand_debian_core.sh || exit $?

mv *${PACKAGE_VERSION}.tar.gz ${PACKAGE_NAME}_${DEBIAN_VERSION}.orig.tar.gz || exit $?

mv debian source/ || exit $?
pushd source || exit $?
debuild -S -sa || exit $?

# cleanup (no, required for testing)
#popd
#rm -rf source
