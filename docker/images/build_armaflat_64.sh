#!/bin/bash

# builds everything required for flatpak builds

#wd="`dirname $0`"
#${wd}/build_armabuild.sh amd64/ubuntu:16.04 armaflat_64 --target armaflat

# SADLY, this does not work in rootless docker.
# This would be the relevant content of Dockerfile.proto:

# ###########################
# flatpak dependencies
# ###########################

FROM armabuild_base as armaflat_base
RUN apt-get -y update && apt-get install -y software-properties-common && \
add-apt-repository ppa:alexlarsson/flatpak && \
apt-get -y update && apt-get install \
flatpak \
flatpak-builder \
-y && \
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
RUN flatpak -y install flathub org.freedesktop.Sdk/x86_64/20.08 flathub org.freedesktop.Platform/x86_64/20.08

FROM armaflat_base as armaflat
# set up builder user
RUN useradd builder && mkdir -p /home/builder && chown builder:builder /home/builder # && chmod 777 /home/builder
WORKDIR /home/builder
USER builder
# bootstap cache
RUN git clone https://gitlab.com/armagetronad/org-armagetronad-ArmagetronAdvanced.git flatpak && \
cd flatpak && \
git checkout legacy_0.2.9 && \
git submodule init && \
git submodule update && \
cd .. && \
flatpak-builder --disable-rofiles-fuse --repo=repo fp_build flatpak/org.armagetronad.ArmagetronAdvanced.json && \
rm -rf .flatpak-builder/build/armagetronad* flatpak repo

# this would be in update_flatpak.sh or similar
git submodule init || exit $?
git submodule update || exit $?
popd

flatpak-builder --repo=repo fp_build flatpak/org.armagetronad.ArmagetronAdvanced.json || exit $?
