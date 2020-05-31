#/bin/bash

#set -x



PACKAGE=${PACKAGE_NAME}
NAME=${PACKAGE_TITLE}


pushd debian
sed -i -e "s/Armagetron Advanced/${NAME}/g" rules
for f in * man/* bin/* source/*; do
    dest=$(echo $f | sed -e "s,armagetronad,${PACKAGE},g")
    if test ! -d $f; then
	echo $f "->" $dest
	sed -e "s/armagetronad/${PACKAGE}/g" < $f | sed -e "s/${PACKAGE}\.net/armagetronad.net/g" -e "s/${PACKAGE}\.source/armagetronad.source/g" -e "s/${PACKAGE}-armagetronad/armagetronad-armagetronad/g" > $dest.out
	rm -f $f $dest
	mv $dest.out $dest
    fi
done
chmod 755 rules
popd

