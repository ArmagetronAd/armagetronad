#!/bin/sh

test $ACLOCAL    || ACLOCAL=aclocal
test $AUTOHEADER || AUTOHEADER=autoheader
test $AUTOCONF   || AUTOCONF=autoconf
test $AUTOMAKE   || AUTOMAKE=automake

# set -x

test -r ChangeLog || touch -t 198001010000 ChangeLog

MYDIR=`dirname $0`
if test -r batch/make/version; then
    echo "Generating version..."
    echo "m4_define(AUTOMATIC_VERSION,["`sh batch/make/version $MYDIR`"])" > version.m4 || exit 1
	sh batch/make/version --verbose $MYDIR | awk '{ print "#define TRUE_ARMAGETRONAD_" $1 " " substr( $0, index( $0, $2 ) ) }' > src/tTrueVersion.h

    # try to fix source epoch
    if test -e .git; then
        if SOURCE_DATE_EPOCH=`git show --pretty='format:%at' -q`; then
            git update-index --refresh > /dev/null
            if git diff-index --quiet HEAD --; then
                echo "m4_define(SOURCE_DATE_EPOCH,${SOURCE_DATE_EPOCH})" >> version.m4 || exit 1
            fi
        fi
    fi

    rm -f version
fi
echo "Copying license..."
test -r COPYING || cp COPYING.txt COPYING || exit $?
echo "Running aclocal..."
$ACLOCAL || { rm aclocal.m4; exit 1; }

echo "Running autoheader..."
rm -f config.h.in
$AUTOHEADER || { rm aa_config.h.in; exit 1; }

echo "Running autoconf..."
$AUTOCONF || { rm configure; exit 1; }

echo "Running automake..."
$AUTOMAKE -a -Wno-portability || { echo "Automake failed"; exit 1; }

echo "Flagging scripts as executable..."
chmod a+x $MYDIR/*.sh || exit 1
echo "Done!  You may now run configure and start building."
