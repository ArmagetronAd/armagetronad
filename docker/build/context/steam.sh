#!/bin/bash

# build a portable application dir for the steam runtime
test -x fakerelease.sh && . fakerelease.sh
. version.sh || exit 1

set -x

STEAM_PACKAGE_NAME=`echo ${PACKAGE_NAME} | sed -e s/-.*//` || exit $?
STEAM_PACKAGE_TITLE=`echo ${PACKAGE_TITLE} | sed -e s/-.*// -e "s/ .*//"` || exit $?

# delegate
./appdir.sh $@ || exit $?

# move stuff around
sed -i appdir/armagetronad.desktop -e s/armagetronad/${STEAM_PACKAGE_NAME}/g || exit $?
if test `cat serverclient` = client; then
    sed -i appdir/AppRun -e s/EXEC=.*\$/EXEC=${PACKAGE_NAME}/g || exit $?
    mv appdir/AppRun appdir/${STEAM_PACKAGE_TITLE} || exit $?
    mv appdir/armagetronad.desktop appdir/${STEAM_PACKAGE_NAME}.desktop || exit $?
else
    sed -i appdir/AppRun -e s/EXEC=.*\$/EXEC=${PACKAGE_NAME}-dedicated/g || exit $?
    mv appdir/AppRun appdir/${STEAM_PACKAGE_TITLE}_Dedicated || exit $?
    mv appdir/armagetronad.desktop appdir/${STEAM_PACKAGE_NAME}-dedicated.desktop || exit $?
fi
    
# remove libraries that are in the steam runtime
pushd appdir/usr/lib/
    for f in *; do
        grep ^$f\$ /usr/base_library_list && rm $f
    done
popd
