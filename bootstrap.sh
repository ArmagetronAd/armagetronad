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
    echo "m4_define(AUTOMATIC_VERSION,["`sh batch/make/version $MYDIR`"])" > version || exit 1
fi
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
