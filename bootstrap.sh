#!/bin/sh

test $ACLOCAL    || ACLOCAL=aclocal
test $AUTOHEADER || AUTOHEADER=autoheader
test $AUTOCONF   || AUTOCONF=autoconf
test $AUTOMAKE   || AUTOMAKE=automake

test -r ChangeLog || touch -t 198001010000 ChangeLog
MYDIR=`dirname $0`
if test -r batch/make/version; then
    echo "Generating version..."
    echo "m4_define(AUTOMATIC_VERSION,["`sh batch/make/version $MYDIR`"])" > version
fi
echo "Running aclocal..."
$ACLOCAL || rm aclocal.m4
echo "Running autoheader..."
rm -f config.h.in
$AUTOHEADER || rm config.h.in
echo "Running autoconf..."
$AUTOCONF || rm configure
echo "Running automake..."
if test -r CVS/Entries; then
    # activate dependency tracking for CVS users
    $AUTOMAKE -a
else
    # deactivate dependency tracking
    rm -f depcomp
    $AUTOMAKE -a -i
fi

echo "Flagging scripts as executable..."
chmod 755 $MYDIR/*.sh
echo "Done!  You may now run configure and start building."
