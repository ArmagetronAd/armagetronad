#!/bin/bash

. targets.sh || exit $?

#echo ${STAGING}+${STEAM_BRANCH}+${ZI_SERIES}
case ${STAGING}+${STEAM_BRANCH}+${ZI_SERIES} in
    *++*)
        echo "No steam branch, no release"
        exit 0
        ;;
    true+*+stable)
        echo "Staging phase of stable build, do release (not activated automatically)"
        ;;
    false+*+stable)
        echo "Deployment should already have happened in the staging phase"
        exit 0
        ;;
    true+*+*)
        echo "Staging phase on a non-release build, do nothing; wait for final deployment"
        exit 0
        ;;
    false+*+*)
        echo "Final deployment of non-release build, do release"
        ;;
    *+*+*)
        echo "Unexpected STAGING+STEAM_BRANCH combination" ${STAGING}+${STEAM_BRANCH}
        exit 1
        ;;
esac

. ./secrets/steamuser.sh || exit $?

# push upload to steam
set -x

# move steam guard token (generated below) to its place
mkdir -p /home/steam/Steam/config || exit $?
mv secrets/config.vdf /home/steam/Steam/config || true

# go to build directory
cd steamcontentbuilder/scripts || exit $?

# switch to configured steam branch
sed -i app_build_1306180.vdf -e "s/^.*setlive.*$/	\"setlive\" \"${STEAM_BRANCH}\"/"

# pretend to be regular user, enough to make steamcmd use a config directory
export USER=steam
export HOME=/home/steam

script_dir=`pwd`
set +x

/home/steam/steamcmd/steamcmd.sh +login "${STEAM_USER}" "${STEAM_PASSWORD}" +run_app_build_http ${script_dir}/app_build_1306180.vdf +quit || exit $?

# If the above command fails, you need to activate steam guard on the deployment machine.
# Run "make steam_guard" in the docker/build directory. Enter the received steam guard
# code when promtped.
#
# Old, manual instructions:
#
# the login above will fail the first time, prompting for a steam guard code.
# comment the line above, uncomment the lines below and store
# the config.vdf file from the .result.deploy_steam.error/steamcontentbuilder/ in
# among the deployment secrets.
#/home/steam/steamcmd/steamcmd.sh +set_steam_guard_code ... "+login ${STEAM_USER} ${STEAM_PASSWORD}" +quit
#cp /home/steam/Steam/config/* ../
#exit 1
#  you can test whether that was successful by running
#/home/steam/steamcmd/steamcmd.sh "+login ${STEAM_USER} ${STEAM_PASSWORD}" +quit


set -x

# cleanup
rm -rf * || exit $?
rm -rf /home/steam/Steam || exit $?
