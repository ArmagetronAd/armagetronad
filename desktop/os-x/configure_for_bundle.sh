#!/bin/bash

# pick the latest protobuf 21.X version
# we do this because 22 introduced the unwanted dependency on abseil
OLD_PROTOBUF="/usr/local/opt/protobuf@21/"
echo OLD_PROTOBUF=${OLD_PROTOBUF}
if ! test -z "${OLD_PROTOBUF}"; then
    export PKG_CONFIG_PATH=${OLD_PROTOBUF}/lib/pkgconfig:${PKG_CONFIG_PATH}
    export PATH=${OLD_PROTOBUF}/bin:${PATH}
    #echo PKG_CONFIG_PATH=${PKG_CONFIG_PATH}
    #echo PATH=${PATH}
fi

export LDFLAGS="-headerpad_max_install_names"

$(dirname $0)/../../configure --disable-restoreold --enable-automakedefaults \
    --disable-useradd --disable-sysinstall --disable-initscripts \
    --disable-uninstall --disable-etc --disable-games \
    --prefix=/Contents \
    --bindir=/Contents/MacOS \
    --datadir=/Contents/Resources \
    --libdir=/Contents/Frameworks \
    "$@"

