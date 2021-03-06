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

rm -rf "${target}"
mkdir -p "${target}"

if ! cp -lrf context/version.sh $@ "${target}"/ > /dev/null 2>&1; then
    ln -f context/version.sh "${target}"/ || { rm -rf "${target}"; exit 1; }
    
    for f in $@; do
        rm -rf "${target}/`basename $f`"
        if test -f "$f"; then
            ln -f "$f" "${target}"/|| { rm -rf "${target}"; exit 1; }
        else
            if which rsync > /dev/null 2>&1; then
                rsync -qav --link-dest="$f" "$f" "${target}/" || { rm -rf "${target}"; exit 1; }
            else
                cp -lrf "$f" "${target}/" || { rm -rf "${target}"; exit 1; }
            fi
        fi
    done
fi

if test -f fakerelease.sh; then
    ln -f fakerelease.sh "${target}/"
fi
