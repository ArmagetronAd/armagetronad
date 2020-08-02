#!/bin/bash

# takes *.md, reformats as .txt and .html

for f in *.md; do
    base=`echo $f | sed -e s,\.md\$,,` || exit $?
    pandoc -f markdown -t plain < $f > ${base}.txt || exit $?
    pandoc -f markdown -t html < $f > ${base}.html || exit $?
done
