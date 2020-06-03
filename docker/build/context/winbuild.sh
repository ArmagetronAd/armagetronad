#!/bin/sh
# makes windows build from source

set -x

Xvfb :99 -screen 1 640x480x8 -nolisten tcp &
DISPLAY=:99
sleep 1
cd winsource
./fromunix.sh || exit $?
cd ..

# comment out for debugging
rm -rf winsource winlibs debug
