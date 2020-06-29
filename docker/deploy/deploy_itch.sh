#!/bin/bash

# put API key into the right place
mkdir -p ~/.config/itch || exit $?
mv secrets/butler_creds ~/.config/itch/ || exit $?
rm -rf secrets || exit $?

. version.sh || exit $?
. targets.sh || exit $?

if test -z "${ITCH_CHANNEL}"; then
	echo "No itch channel, no release"
	exit 0
fi

if test "${STAGING}" = true; then
	echo "Staging, no release"
	exit 0
fi

CHANNEL_SUFFIX=-${ITCH_CHANNEL}
if test "${ITCH_CHANNEL}" = stable; then
	CHANNEL_SUFFIX=""
fi

# push upload to itch
set -x

# install itch.io butler under local user
# butler gets frequent updates, that is why we do not bake it into the docker images
mkdir -p ~/bin || exit $?
pushd ~/bin || exit $?
if ! curl -L -o butler.zip https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default; then
 sleep 30
 curl -L -o butler.zip https://broth.itch.ovh/butler/linux-amd64/LATEST/archive/default || exit $?
fi
unzip butler.zip || exit $?
rm -f butler.zip || exit $?
chmod +x butler || exit $?
./butler -V || exit $?
popd || exit $?

# upload
if ! ~/bin/butler push appdir_linux_32 ${ITCH_PROJECT}:linux-32${CHANNEL_SUFFIX} --userversion=${PACKAGE_VERSION} --if-changed; then
  sleep 30
  ~/bin/butler push appdir_linux_32 ${ITCH_PROJECT}:linux-32${CHANNEL_SUFFIX} --userversion=${PACKAGE_VERSION} --if-changed || exit $?
fi
~/bin/butler push appdir_linux_64 ${ITCH_PROJECT}:linux-64${CHANNEL_SUFFIX} --userversion=${PACKAGE_VERSION} --if-changed || exit $?
~/bin/butler push appdir_windows ${ITCH_PROJECT}:windows-32${CHANNEL_SUFFIX} --userversion=${PACKAGE_VERSION} --if-changed || exit $?

# cleanup
rm -rf secrets ~/.config/itch appdir_*