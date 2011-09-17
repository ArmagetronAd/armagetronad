#!/bin/bash

# Complies Windows client and server from Linux with Wine set up and
# code::blocks installed therein in the default directory.
# Only works from within unpacked source zip since it skips the
# version generation step.

# compile protobuf (lazily)
pushd ../src/protobuf
ls *.pb.cc *.pb.h || wine ../../../winlibs/protobuf/bin/protoc *.proto --cpp_out=. || exit 1
popd

pushd code_blocks
# batch build sans master
grep -v Master.cbp < ArmagetronAd.workspace > ArmagetronAdNoMaster.workspace
wine "C:\Program Files\CodeBlocks\codeblocks.exe" /ns /nd ArmagetronAdNoMaster.workspace --build --target="Win32 Release" || exit 1
popd
wait

# copy files
echo -e "makedist.bat\n exit" | wine cmd

true


