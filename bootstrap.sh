#!/bin/sh

test $ACLOCAL    || ACLOCAL=aclocal
test $AUTOHEADER || AUTOHEADER=autoheader
test $AUTOCONF   || AUTOCONF=autoconf
test $AUTOMAKE   || AUTOMAKE=automake

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
$AUTOHEADER || { rm config.h.in; exit 1; }

echo "Running autoconf..."
$AUTOCONF || { rm configure; exit 1; }

echo "Running automake..."
automake -a || exit 1

echo "Flagging scripts as executable..."
chmod 755 $MYDIR/*.sh || exit 1

echo "Done!  You may now run configure and start building."
