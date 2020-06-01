# Docker Based Build System

## You Need to Have Installed
 * Everything needed for a dedicated server build, plus python packaging
 * [docker in rootless mode](https://docs.docker.com/engine/security/rootless/)
 * Optional: [docker in reglar mode](https://docs.docker.com/engine/install/ubuntu/) for steam SDK creation
 * Optional: [x11docker](https://github.com/mviereck/x11docker) for Windows build system creation

Ubuntu is recommended, as rootless docker supports the overlay2 driver there;
pretty much everyone else only gets the vfs driver that always copies the whole
image contents.

For Ubuntu, to be on the safe side:

    sudo apt install git wine-stable automake \
    bison g++ make libboost-dev libboost-thread-dev \
    libprotobuf-dev libzthread-dev python recode curl uidmap python3-packaging

Then follow the instructions from the rootless docker and x11docker above.

## Contents

 * scripts/ contains helper scripts
 * images/ contains everything else needed to build images from scratch
 * build/ contains the actual build stuff
 * deploy/ contains information about deployments; pre-filled with target definition; each requires certain secrets

## How to Build

Assuming the source is in armagetronad, do

    cd armagetronad
    ./bootstrap.sh
    mkdir ../build
    cd ../build
    ../armagetronad/configure --disable-glout
    cd docker/build
    make <target>

target can be

   * *free*: Build everything relevant for free open source systems
   * *closed*: Build steam and windows binaries
   * *full*: Build all
   * *clean*: Remove build results

## Developer Notes
To update to another branch, you may have to adapt related branch names in scripts/relevant_branches.sh.

To make builds use up to date versions of additional git repositories, call scripts/update_gits.sh.

All scripts can be invoked from any work directory as long as you invoke them with a relative or absoulte path, they find their relevant data directories and other scripts automatically.