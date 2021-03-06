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

If you are not using Ubuntu on your main machine, you can run Rootless Docker inside
an Ubuntu virtual machine and export the socket for use on your main machine. The safest way to run such a machine is with one NAT network connection that you enable
a port forwarding [rule for SSH](https://bobcares.com/blog/virtualbox-ssh-nat/) for.
Once that is set up, assuming your UID is 1000 on the host and 1001 on the host and you went with the example 2522 source port, connect to the guest with

    ssh -p 2522 guest -L /run/user/1001/docker.sock:/run/user/1000/docker.sock

as suggested [here](https://docs.docker.com/engine/security/security/#docker-daemon-attack-surface) and keep that running. Inform your local docker client about the socket with

    export DOCKER_HOST=unix:///run/user/1000/docker.sock

and off you go. Only your host user can access that socket and can only control a Docker daemon on a virtual machine running as a non-root user.

To secure the VM some more, you may want to restrict its access to your LAN. You can use [ufw](https://linuxize.com/post/how-to-setup-a-firewall-with-ufw-on-ubuntu-20-04/) for that. Assuming 192.160.0.1 is your router's IP on the LAN, execute on the VM:

    sudo ufw allow ssh                      # default is to block everything, so before we enable ufw, we better make sure ssh will keep working
    sudo ufw enable
    sudo ufw allow out to 192.168.0.1       # this allows traffic to your router and therefore the internet
    sudo ufw deny out to 10.0.0.0/8         # these block all private subnets; LANs usually use these IPs. If yours does not, adapt.
    sudo ufw deny out to 172.16.0.0/12
    sudo ufw deny out to 192.168.0.0/16
    sudo ufw deny in from 10.0.0.0/8
    sudo ufw deny in from 172.16.0.0/12
    sudo ufw deny in from 192.168.0.0/16

The 'deny in' rules are redundant because the VM is behind a NAT anyway, but you never know. Maybe you switch it to 'bridged' one day.

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

## GitLab CI

The root .gitlab-ci.yml uses the files in this directory to do the builds. The building should work fine in any gitlab runner configured to use docker, especially the shared runners.

For deployment, however, an own runner needs to be set up. A stadard setup will do fine, but then run as root. To run a rootless gitlab runner, you must do the following. Install gitlab runner normally, but disable the default service with

    systemctl disable gitlab-runner

As a regular user, register a runner with

    gitlab-runner register

the data to fill in is described on the GitLab CI configuration of the project. Then, normally, gitlab-runner uses docker-in-docker to run additional containers. That does not work rootless. Instead, you can just make the docker control socket available to gitlab's container by editing the volumes line in `~/.gitlab-runer/config.toml` to

    volumes = ["/var/run/user/UID/docker.sock:/var/run/docker.sock", "/cache", "/home/USERNAME/secrets:/secrets:ro"]

replacing UID with your user ID and USERNAME with your username. If you want to use the runner for deployment, the secrets folder then has to contain the credentials required to do so. Read deploy/targets.sh for details.

Then, run the runner with

    gitlab-runner run

 Security and consistency implication: You then only have one docker instance. Jobs running on your runner can therefore modify the same docker images, for example apply tags. Therefore, all our CI scripts reference used images by their digest and use randomized names for containers so they don't get into one another's way.

 ## macOS

 macOS builds happen from this directory, too, without the need for docker. Of course, WITH docker, you can do all the other builds on a Mac.
 Prerequisites:

    brew install pkg-config autoconf automake sdl sdl_image sdl2 sdl2_image sdl2_mixer protobuf-c glew boost ftgl dylibbundler create-dmg wget python3

For a GitLab runner, choose a VirtualBox based runner. Use [this](https://github.com/myspaghetti/macos-virtualbox) script to create a Mojave VM,
then follow the [runner installation instructions](https://docs.gitlab.com/runner/install/osx.html). An important preparation step for building the
.dmg files correctly is that the builder user needs the rights to remote control Finder; for that, just ssh into the machine as the build user (can be from
within the VM itself) and do a test compilation run.

The macOS builds are done with

    cd docker/build
    make macOS
