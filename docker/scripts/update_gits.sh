#!/bin/bash

# updates git 'submodules', hardocdes the current revision into fix_gits.sh script

wd=`dirname $0`
download_dir="${wd}/.cache/gits"
. ${wd}/relevant_branches.sh || exit $?

branch=${MAIN_BRANCH}
${wd}/ensure_gits.sh ${branch} || exit $?

of=${wd}/fix_gits.sh

function fix_git(){
    name=$1

    dir=${download_dir}/${name}
    git -C ${dir} fetch || return $?
    git -C ${dir} pull || return $?
    rev=`git -C ${dir} rev-parse HEAD`

    echo "    fix_git $1 ${rev} || return \$?" >> ${of}

    return 0
}

cat > ${of} <<EOF
#!/bin/bash
wd=\`dirname \$0\`
download_dir="\${wd}/.cache/gits"

function fix_git(){
    name=\$1
    rev=\$2    

    dir=\${download_dir}/\${name}
    if ! git -C \${dir} reset \${rev} --hard; then
        git -C \${dir} fetch
	git -C \${dir} reset \${rev} --hard || return \$?
    fi
}

function fix_gits(){
    \${wd}/ensure_gits.sh ${branch} || return \$?

    set -x

EOF

fix_git macOS
fix_git winlibs
fix_git codeblocks
fix_git ubuntu

cat >> ${of} <<EOF
}

if ! fix_gits; then
    # clean and retry on error
    rm -rf \${download_dir}
    fix_gits || exit \$?
fi
EOF

chmod 755 ${of}

${of}

#set -x

# fetch rebrand script from ubuntu daily builds
rd=${wd}/../build/rebrand_debian_core.sh
git -C ${download_dir}/ubuntu show origin/${UBUNTU_BRANCH}-dailydeb:rebrand.sh |\
    sed -e s/PACKAGE=.*/PACKAGE=\${PACKAGE_NAME}/ \
	-e s/NAME=.*/NAME=\${PACKAGE_TITLE}/ \
	-e "s/pushd.*/pushd debian/" |\
    grep -v topdir | grep -v branch > ${rd} >\
	 ${rd}

#cat ${rd}
chmod 755 ${rd}


