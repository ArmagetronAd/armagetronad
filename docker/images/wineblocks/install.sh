#!/bin/bash
wine ./download/codeblocks-setup1312.exe || exit 1
wine ./download/nsis-setup304.exe || exit 1

# code::blocks crashes on exit, the crash dialog would block our builds. Disable it.
sh ./download/winetricks nocrashdialog || exit 1
