#!/bin/bash

# canonical 64 bit steam SDK environment
# see https://github.com/ValveSoftware/steam-runtime#building-in-the-runtime

# outdated, can go away on next update, see build_armasteam_64.sh

set -x

wd="`dirname $0`"
bd="${wd}/context/steambase"
sd="${wd}/../scripts"
rm -rf ${bd}
mkdir -p ${bd}

. ${wd}/epoch.sh

sdk=com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.tar.gz
dockerfile=com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.Dockerfile
release_dir=http://repo.steampowered.com/steamrt-images-scout/snapshots/0.20200318.2

${sd}/download.sh ${bd}/${sdk} ${release_dir}/${sdk}
${sd}/download.sh ${bd}/${dockerfile} ${release_dir}/${dockerfile}

scout=${REGISTRY}steamrt_scout_amd64:${EPOCH}

pushd "${bd}"
if ! docker build \
 -f com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.Dockerfile \
 -t ${scout} \
 . || exit 1; then
    sudo docker build \
	 -f com.valvesoftware.SteamRuntime.Sdk-amd64,i386-scout-sysroot.Dockerfile \
	 -t ${scout} \
	 . || exit 1
    sudo docker save -o steamrt.tar ${scout} || exit 1
    sudo docker rmi -f ${scout} || exit 1
    sudo chown "$USER" steamrt.tar || exit 1
    docker load -i steamrt.tar || exit 1
    rm -f steamrt.tar
fi

popd



