#!/bin/sh

test -r ChangeLog || touch touch -t 198001010000 ChangeLog
MYDIR=`dirname $0`
if test -r batch/make/version; then
    echo "Generating version..."
    echo "m4_define(AUTOMATIC_VERSION,["`sh batch/make/version $MYDIR`"])" > version
fi
echo "Running aclocal..."
aclocal || rm aclocal.m4
echo "Running autoheader..."
rm -f config.h.in
autoheader || rm config.h.in
echo "Running autoconf..."
autoconf || rm configure
echo "Running automake..."
if test -r CVS/Entries || test -r .svn/entries; then
    # activate dependency tracking for CVS and Subversion users
    automake -a
else
    # deactivate dependency tracking
    rm -f depcomp
    automake -a -i
fi

echo "Flagging scripts as executable..."
chmod 755 $MYDIR/*.sh $MYDIR/*-sh
echo "Done!  You may now run configure and start building."
