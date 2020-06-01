#!/bin/bash

# prepares a context for a docker based build
# usage: prepare_context.sh <target_dir> <files and directories>

target="$1"
shift

#echo -e "\e]2;Prepare ${target}\a"

echo -e "\n\n********************************************************************"
echo -e "Prepare ${target} from $1"
echo -e "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\n"

set -x

rm -rf ${target}
mkdir -p ${target}

if ! cp -lrf context/version.sh $@ ${target}/; then
    rm -rf ${target}/version.sh
    cp -lrf context/version.sh ${target}/ || { rm -rf ${target}; exit 1; }
    
    for f in $@; do
	rm -rf ${target}/`basename $f`
	cp -lrf $f ${target}/ || { rm -rf ${target}; exit 1; }
    done
fi

if test -f fakerelease.sh; then
    rm -rf ${target}/fakerelease.sh
    cp -lrf fakerelease.sh ${target}/
fi
