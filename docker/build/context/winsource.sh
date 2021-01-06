#!/bin/sh
# packs windows sources

set -x

. ./version.sh

WINDIR=winsource
SRCDIR=source
BUILDDIR=build
DOCDIR=build/src/doc
CODEBLOCKS=codeblocks

# prepare windows sources
rm -rf $WINDIR
cp -a $SRCDIR $WINDIR || exit 1
rm -rf $WINDIR/src/doc
rm -rf $WINDIR/doc
cp -r $DOCDIR $WINDIR/doc || exit 1
cp -r $BUILDDIR/language/languages.txt $WINDIR/language || exit 1
cp -r $BUILDDIR/config/aiplayers.cfg $WINDIR/config || exit 1

# rename text files
for f in $WINDIR/INSTALL $WINDIR/COPYING $WINDIR/AUTHORS $WINDIR/NEWS $WINDIR/ChangeLog `find $WINDIR -name "*README*"`; do
    mv $f $f.txt || exit 1
done

# copy Code::Blocks build files
if test -n "${CODEBLOCKS}" && test -d ${CODEBLOCKS}; then
    cp -r ${CODEBLOCKS}/* ${WINDIR}/ || exit 1
    
    # adapt relative paths to source
    for f in `ls ${CODEBLOCKS} -1 | egrep ".*\.cbp$|.*.\.bat$"`; do
        echo $f
	sed < ${CODEBLOCKS}/$f > ${WINDIR}/$f -e "s,=\.\.\\\\armagetronad,=.,g" -e "s,\.\./armagetronad[^_],,g" -e "s,\.\.\\\\armagetronad[^_],,g" -e "s,armagetronad,${PACKAGE_NAME},g" || exit $?
    done
fi

# rebrand storage directory
STORAGE=${PACKAGE_TITLE}
test "${STORAGE}" = "Armagetron Advanced" && STORAGE="Armagetron" # 'Armagetron' used to be the only storage directory name before
sed -i ${WINDIR}/src/win32/config_common.h -e "s,\}/Armagetron\",}/${STORAGE}\"," || exit $?

# transcribe nsi scripts
for source in ${CODEBLOCKS}/*.nsi; do
    f=`basename ${source}`
    #echo Adapting installer script $f...
    sed < ${source} > ${WINDIR}/$f \
        -e "s,define PRODUCT_VERSION.*$,define PRODUCT_VERSION \"${PACKAGE_VERSION}\"," \
        -e "s,define PRODUCT_BASENAME.*$,define PRODUCT_BASENAME \"${PACKAGE_TITLE}\"," \
	-e "s,armagetronad,${PACKAGE_NAME},g" || exit 1
	
    for suffix in gcc; do
	sed < ${source} > ${WINDIR}/${suffix}.$f \
            -e "s,define PRODUCT_VERSION.*$,define PRODUCT_VERSION \"${PACKAGE_VERSION}.${suffix}\"," \
            -e "s,define PRODUCT_BASENAME.*$,define PRODUCT_BASENAME \"${PACKAGE_TITLE}\"," \
	    -e "s,armagetronad,${PACKAGE_NAME},g" || exit 1	    
	
        # generate special makedist script that copies only the wanted installer generators
        MAKEDIST=${WINDIR}/makedist_${suffix}.bat
        echo "call makedist.bat" > ${MAKEDIST}
        echo "xcopy dist\\${suffix}.*.nsi . /Y" >> ${MAKEDIST}
        echo "del dist\\*.nsi" >> ${MAKEDIST}
        echo "xcopy ${suffix}.*.nsi dist /Y" >> ${MAKEDIST}
        echo "del ${suffix}*.nsi" >> ${MAKEDIST}
        echo "pause" >> ${MAKEDIST}
    done
done

# transcode to windows CR/LF line mode
for suffix in bat nsi nsi.m4 cfg txt dsp dsw h cpp dtd xml; do
	find $WINDIR -name "*.$suffix" -exec recode latin1..latin1/CR-LF \{\} \;
done

rm -rf $WINDIR/config.* $WINDIR/configure* $WINDIR/*.m4 $WINDIR/*.kde* 
rm -rf $WINDIR/bootstrap.sh $WINDIR/batch $WINDIR/missing $WINDIR/install-sh
find $WINDIR -name "*.in" -exec rm \{\} \;
find $WINDIR -name "*.am" -exec rm \{\} \;
find $WINDIR -name "Makefile*" -exec rm \{\} \;
find $WINDIR -name "*.ghost" -exec rm \{\} \;
find $WINDIR -depth -name "CVS" -exec rm -rf \{\} \;
find $WINDIR -depth -name "*~" -exec rm -rf \{\} \;

chmod +x winsource/fromunix.sh || exit 1

# comment out for debugging
rm -rf ${CODEBLOCKS} ${SRCDIR} ${BUILDDIR}
