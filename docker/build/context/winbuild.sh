#!/bin/sh
# makes windows build from source

set -x

Xvfb :99 -screen 1 640x480x8 -nolisten tcp &
export DISPLAY=:99
sleep 1
cd winsource/win32
./fromunix.sh || exit $?
cd ../../
mv winsource/build/dist .

# comment out for debugging
# rm -rf winsource
