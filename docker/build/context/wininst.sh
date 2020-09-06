#!/bin/sh
# builds windows installer

set -x

if test -r ~/.wine/drive_c/Program\ Files\ \(x86\)/NSIS/makensis.exe; then
    NSIS="C:\Program Files (x86)\NSIS\makensis.exe"
else
    NSIS="C:\Program Files\NSIS\makensis.exe"
fi

Xvfb :99 -screen 1 640x480x8 -nolisten tcp &
DISPLAY=:99
sleep 1
cd dist
wine "$NSIS" armagetronad.nsi || exit $?
wine "$NSIS" armagetronad_dedicated.nsi || exit $?
mv *.win32.exe ../ || exit $?
cd ..

# comment out for debugging
rm -rf dist
