#!/bin/bash

# brands and series according to git repository and CI environment

git_repo=$1

BRANCH=

if test -z "${CI_DEFAULT_BRANCH}" || test -z "${CI_COMMIT_REF_PROTECTED}"; then
	# no CI, get info from git directly
	CI_COMMIT_REF_PROTECTED=false
	CI_DEFAULT_BRANCH=trunk
	CI_COMMIT_SHA=`git -C ${git_repo} rev-parse HEAD`
	CI_COMMIT_BRANCH=`git -C ${git_repo} rev-parse --abbrev-ref HEAD`
	CI_COMMIT_REF_NAME=${CI_COMMIT_BRANCH}
	CI_COMMIT_TAG=
	CI_MERGE_REQUEST_CHANGED_PAGE_PATHS=
	CI_MERGE_REQUEST_ID=
	CI_MERGE_REQUEST_SOURCE_BRANCH_NAME=
	CI_MERGE_REQUEST_TARGET_BRANCH_NAME=
	unix_date=`git -C ${git_repo} log -1 --format=%at` || exit $?
	CI_RELEASE_YEAR=`date --date=@${unix_date} +%Y` || exit $?
else
	# CI_COMMIT_TIMESTAMP is of the form 2020-12-21T02:13:17+01:00
	CI_RELEASE_YEAR=`echo ${CI_COMMIT_TIMESTAMP} | sed -e "s/-.*//"` || exit $?
fi

if test "${CI_COMMIT_REF_PROTECTED}" = "true"; then
	SERIES="CURRENT"
	if test -z "${CI_COMMIT_TAG}"; then
		# protected branch
		case ${CI_COMMIT_BRANCH} in
			release*)
			PROGRAM_NAME="armagetronad"
			PROGRAM_TITLE="Armagetron Advanced"
			;;
			beta*)
			PROGRAM_NAME="armagetronad-beta"
			PROGRAM_TITLE="Armagetron Beta"
			;;
			legacy_*_*)
			SERIES="WIP"
			PROGRAM_NAME="armagetronad-wip"
			PROGRAM_TITLE="Armagetron WIP"
			;;
		    legacy*)
			PROGRAM_NAME="armagetronad-alpha"
			PROGRAM_TITLE="Armagetron Alpha"
			;;
		    trunk)
			SERIES="EXPERIMENTAL"
			PROGRAM_NAME="armagetronad-experimental"
			PROGRAM_TITLE="Armagetron Experimental"
			;;
			hack*)
			SERIES="HACK"
			PROGRAM_NAME="armagetronad-hack"
			PROGRAM_TITLE="Armagetron Hack"
			;;
			*)
			SERIES="WIP"
			PROGRAM_NAME="armagetronad-wip"
			PROGRAM_TITLE="Armagetron WIP"
			;;
		esac
	else
		# protected tag, must be a release
		PROGRAM_NAME="armagetronad"
		PROGRAM_TITLE="Armagetron Advanced"
	fi
else
	# unknown unprotected build
	SERIES="WIP"
	PROGRAM_NAME="armagetronad-wip"
	PROGRAM_TITLE="Armagetron WIP"
fi
