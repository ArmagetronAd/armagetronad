#!/bin/bash

# just tries to log in to steam

. targets.sh || exit $?
. ./secrets/steamuser.sh || exit $?

set -x

# go to build directory
cd steamcontentbuilder/scripts || exit $?

script_dir=`pwd`
set +x

if /home/steam/steamcmd/steamcmd.sh +login "${STEAM_USER}" "${STEAM_PASSWORD}" +run_app_build_http ${script_dir}/app_build_1306180.vdf +quit; then
    exit 1
fi

# the login above will fail the first time, prompting for a steam guard code.
# run obtain_steam_guard_2.sh with the guard code to get config.vdf.

# cleanup
cd ../../ || exit $?
rm -rf * || exit $?
