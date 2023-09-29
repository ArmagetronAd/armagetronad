#!/bin/bash

# just tries to log in to steam

. targets.sh || exit $?
. ./secrets/steamuser.sh || exit $?
. ./secrets/steamguard.sh || exit $?

set -x

# go to build directory
cd steamcontentbuilder/scripts || exit $?

script_dir=`pwd`
set +x

# the last login failed the first time, prompting for a steam guard code.
# if this is successful, copy the config.vdf file from the .result.deploy_steam.error/steamcontentbuilder/ in
# among the deployment secrets.
/home/steam/steamcmd/steamcmd.sh +set_steam_guard_code ${STEAM_GUARD_CODE} "+login ${STEAM_USER} ${STEAM_PASSWORD}" +quit || exit $?
cp /home/steam/Steam/config/* ../output
#  you can test whether that was successful by running
/home/steam/steamcmd/steamcmd.sh "+login ${STEAM_USER} ${STEAM_PASSWORD}" +quit || exit $?

# cleanup
mv ../output /home/steam/ || exit $?
cd ../../ || exit $?
rm -rf * || exit $?
mv ../output . || exit $?
