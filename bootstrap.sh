#!/bin/sh

test -r ChangeLog || touch -t 198001010000 ChangeLog
MYDIR=`dirname $0`
if test -r batch/make/version; then
    echo "Generating version..."
    echo "m4_define(AUTOMATIC_VERSION,["`sh batch/make/version $MYDIR`"])" > version || exit 1
fi
echo "Running aclocal..."
aclocal || { rm aclocal.m4; exit 1; }
echo "Running autoheader..."
rm -f config.h.in
autoheader || { rm config.h.in; exit 1; }
echo "Running autoconf..."
autoconf || { rm configure; exit 1; }
echo "Running automake..."
automake -a || exit 1

echo "Flagging scripts as executable..."
chmod a+x $MYDIR/*.sh || exit 1
echo "Done!  You may now run configure and start building."
