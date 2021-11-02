#!/bin/bash

# waits for the download of an URI to become active.
# Some services (LD, SF) have a delay between upload and availability,
# we do not want initially dead download links to go live.

URI="$1"

timeout=10
while test ${timeout} -gt -1; do
    if curl ${URI} --fail --silent -k --show-error -L -o /dev/null; then
        exit 0 # success
    elif test ${timeout} -gt 0; then
        echo "Waiting for download ${URI}: ${timeout}"
        sleep 120
    fi
    timeout=$(( ${timeout} - 1 ))
done

# timeout
exit 1
