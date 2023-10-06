#!/bin/bash

. targets.sh || exit $?

#echo ${STAGING}+${STEAM_BRANCH}+${ZI_SERIES}
case ${STAGING}+${STEAM_BRANCH}+${ZI_SERIES} in
    *++*)
	echo "No steam branch, no release"
	exit 0
	;;
    true+*+stable)
	# put up on staging branch
	;;
    false+*+stable)
	# staging already happened when STAGING was true
	exit 0
	;;	
    true+*+*)
	# just staging, do not put up on steam yet
	exit 0
	;;
    false+*+*)
	# final release of that branch, deploy
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
mv ../output /home/steam/ || exit $?
cd ../../ || exit $?
rm -rf * || exit $?
mv ../output . || exit $?
