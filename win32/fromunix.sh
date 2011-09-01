#!/bin/bash

# Complies Windows client and server from Linux with Wine set up and
# code::blocks installed therein in the default directory.
# Only works from within unpacked source zip since it skips the
# version generation step.

# compile protobuf
pushd ../src/protobuf
wine ../../../winlibs/protobuf/bin/protoc *.proto --cpp_out=. || exit 1
popd

pushd code_blocks
# batch build (assume winlibs is completely built)
#wine "C:\Program Files\CodeBlocks\codeblocks.exe" /ns /nd ArmagetronAd.cbp --build --target=release
#wine "C:\Program Files\CodeBlocks\codeblocks.exe" /ns /nd Dedicated.cbp --build --target=release
wine "C:\Program Files\CodeBlocks\codeblocks.exe" /ns /nd ArmagetronAd.workspace --build --target="Win32 Release" || exit 1
popd
wait

# copy files
echo -e "makedist.bat\n exit" | wine cmd

true


