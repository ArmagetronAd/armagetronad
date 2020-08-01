#!/bin/bash

# push make an entry on the download site

set +x

mv secrets/ssh ~/.ssh

# remove other secrets
rm -rf secrets/*

set -x

. ./version.sh || exit $?
. ./targets.sh || exit $?

dd=`dirname $0`

# determine package names
pushd upload || exit $?
for f in *; do
    echo $f
    case $f in
	*-dedicated-*.exe)
	    WIN_SERVER=$f
	    ;;
	*.exe)
	    WIN_CLIENT=$f
	    ;;
	*Dedicated-32bit-${PACKAGE_VERSION}.AppImage)
	    LIN32_SERVER=$f
	    ;;
	*-32bit-${PACKAGE_VERSION}.AppImage)
	    LIN32_CLIENT=$f
	    ;;
	*Dedicated-${PACKAGE_VERSION}.AppImage)
	    LIN64_SERVER=$f
	    ;;
	*-${PACKAGE_VERSION}.AppImage)
	    LIN64_CLIENT=$f
	    ;;
	*-client*.tbz|*-server*.tbz)
	    ;;
	*-${PACKAGE_VERSION}.tbz)
	    SOURCE_TARBALL=$f
	    ;;
	*win32.zip)
	    ;;
	*-${PACKAGE_VERSION}.zip)
	    SOURCE_ZIP=$f
	    ;;
    esac
done
popd

# fetch site from git
trust_gitlab || exit $?
git clone ${DOWNLOAD_SITE_GIT} site || exit $?
pushd site || exit $?

TITLE="Build ${PACKAGE_VERSION} available"
DL_BRANCH=${ZI_SERIES}

BUILD_TYPE=build
if test "${CI_COMMIT_REF_PROTECTED}" == "true" && test "${ZI_SERIES}" == "stable"; then
	# only releases have no _rc_, _alpha_ or _beta_ in their version
	if ! echo ${PACKAGE_VERSION} | grep -q '_[a-z]*_'; then
		BUILD_TYPE="release build"
	fi
fi

# add release post
POST=_posts/`date +%F`-build-${DL_BRANCH}-${PACKAGE_VERSION}.md || exit $?
cat > ${POST} <<EOF
---
layout: post
title:  "${TITLE}"
date:   `date +"%F %T %z" -u`
categories: ${BUILD_TYPE} ${DL_BRANCH}

version: ${PACKAGE_VERSION}

series: ${SERIES}
suffix: ${SUFFIX}
git_rev: ${CI_COMMIT_SHA}
git_reference: ${CI_COMMIT_REF_NAME}

zeroinstall_branch: ${ZI_SERIES}
steam_branch: ${STEAM_BRANCH}
ppa_branch: ${SUFFIX}

uri_base: ${DOWNLOAD_URI_BASE}

uri_winclient: ${WIN_CLIENT}
uri_lin64client: ${LIN64_CLIENT}
uri_lin32client: ${LIN32_CLIENT}

uri_winserver: ${WIN_SERVER}
uri_lin64server: ${LIN64_SERVER}
uri_lin32server: ${LIN32_SERVER}

uri_tarsrc: ${SOURCE_TARBALL}
uri_zipsrc: ${SOURCE_ZIP}

---

{% include ${DL_BRANCH}_build_v1.md %}

EOF

if test -r ../upload/RELEASENOTES.md; then
	cat >> ${POST} <<EOF
### Release Notes

EOF
	cat ../upload/RELEASENOTES.md >> ${POST}
	echo >> ${POST}
fi

if test -r ../upload/PATCHNOTES.md; then
	cat >> ${POST} <<EOF
### Patch Notes

EOF
	cat ../upload/PATCHNOTES.md >> ${POST}
fi

CURRENT=_includes/current_${SERIES}_${DL_BRANCH}.md || exit $?
cat > ${CURRENT} <<EOF
{% assign ${DL_BRANCH}_version = "${PACKAGE_VERSION}" %}
{% capture ${DL_BRANCH}_link %}{% link ${POST} %}{% endcapture %}
EOF

# update git
git add ${CURRENT} ${POST}
git pull || exit $?
git commit -m "Update to ${PACKAGE_VERSION}" || exit $?

if test "${STAGING}" == "true"; then
    echo "Only staging, no real update of the website"
else
	git push || exit $?
fi

# cleanup
rm -rf source upload secrets secrets.*

exit ${EXIT}
