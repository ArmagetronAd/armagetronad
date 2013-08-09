#!/bin/bash
# A script that simply echos back ladderlog lines to the server console.

while read line
do
    echo "# $line"
done
