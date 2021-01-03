#!/bin/sh

# set -x

test -r ChangeLog || touch -t 198001010000 ChangeLog
MYDIR=`dirname $0`
if test -r batch/make/version; then
    echo "Generating version..."
    echo "m4_define(AUTOMATIC_VERSION,["`sh batch/make/version $MYDIR`"])" > version.m4 || exit 1

    # try to fix source epoch
    if test -e .git; then
        if SOURCE_DATE_EPOCH=`git log --date=unix | grep Date: | head -n 1 | sed -e "s,Date: *,,"`; then
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
aclocal || { rm aclocal.m4; exit 1; }
echo "Running autoheader..."
rm -f config.h.in
autoheader || { rm config.h.in; exit 1; }
echo "Running autoconf..."
autoconf || { rm configure; exit 1; }
echo "Running automake..."
automake -a -Wno-portability || exit 1

echo "Flagging scripts as executable..."
chmod a+x $MYDIR/*.sh || exit 1
echo "Done!  You may now run configure and start building."
