#!/bin/bash

# just tries to log in to steam

. ./targets.sh || exit $?
. ./secrets/steamuser.sh || exit $?

set -x

# pretend to be regular user, enough to make steamcmd use a config directory
export USER=steam
export HOME=/home/steam

set +x

if /home/steam/steamcmd/steamcmd.sh +login "${STEAM_USER}" "${STEAM_PASSWORD}" +quit; then
    exit 1
fi
set -x

# the login above will fail the first time, prompting for a steam guard code.
# run obtain_steam_guard_2.sh with the guard code to get config.vdf.

# cleanup
rm -rf * || exit $?
