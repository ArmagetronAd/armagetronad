#!/bin/bash

$(dirname $0)/../../configure --disable-restoreold --enable-automakedefaults \
    --disable-useradd --disable-sysinstall --disable-initscripts \
    --disable-uninstall --disable-etc --disable-games \
    --prefix=/Contents \
    --bindir=/Contents/MacOS \
    --datadir=/Contents/Resources \
    --libdir=/Contents/Frameworks \
    "$@"