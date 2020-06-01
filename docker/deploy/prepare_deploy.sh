#!/bin/bash

# prepares deployment

# test that everything has been prepared correctly
if ! test -d gitlab_build; then
    echo "No build yet!"
    exit 1
fi

# make sure everything outside of the build directory is older than anything inside
find . -newer timestamp.tag ! -path "./gitlab_build" -exec touch -r timestamp.tag \{\} \;

# verify that everything has been made 
if ! make -C gitlab_build/docker/build -q CI.tag; then
    make -d -C gitlab_build/docker/build -q CI.tag || exit $?
    exit 1
fi

# security audit-ish: secrets must not be writable
if echo x 2> /dev/null 1> /secrets/test_is_mount_writable; then
    echo "Secret storage directory is writable. Abort."
    exit 1
fi

# We expect the secrets to be mounted by the special deploy runner at /secrets
cp -a /secrets gitlab_build/docker/deploy/ || exit $?

# set git identity
git config --global user.email "z-man@users.sf.net" || exit $?
git config --global user.name "Manuel Moos (From GitLab CI)" || exit $?
