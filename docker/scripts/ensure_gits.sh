#!/bin/bash

# makes sure all git 'submodules' are present in .cache
# optional argument: branch to check out

wd=`dirname $0`
download_dir="${wd}/.cache/gits"
mkdir -p ${download_dir} || exit $?
. ${wd}/relevant_branches.sh || exit $?

branch="$1"
test -z "${branch}" && branch=${MAIN_BRANCH}

function ensure_git(){
    name=$1
    uri=$2
    my_branch=$3
    test -z "${my_branch}" && my_branch=${branch}
    test -z "${my_branch}" && exit 1
    
    dir=${download_dir}/${name}
    if ! test -e ${dir}/.git; then
	rm -rf ${dir}
    fi
    if ! test -d ${dir}; then
	set -x
	git clone ${uri} -b ${my_branch} ${dir} || return $?
    else
	git -C ${dir} checkout ${my_branch} || return $?
    fi

    return 0
}

ensure_git winlibs https://gitlab.com/armagetronad/winlibs.git || exit $?
ensure_git codeblocks https://gitlab.com/armagetronad/build_codeblocks.git || exit $?
ensure_git steam-art https://gitlab.com/armagetronad/steam-art.git master || exit $?
ensure_git pkg2appimage https://github.com/AppImage/pkg2appimage.git master || exit $?

