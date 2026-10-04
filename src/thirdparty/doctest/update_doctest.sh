#!/usr/bin/env bash

# Download the latest version of the doctest header.
# Doctest is a single header testing framework and this is the easiest way to integrate it.
# Doctest itself is distributed under the MIT license, that still applies if you extract the copy
# found here.

cd `dirname $0`
wget -O doctest.h https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h
