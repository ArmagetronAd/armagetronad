#!/bin/bash

# tar is weird, returing 1 on a non-error (content changed)
# try several times, ignore 1

tar $* || tar $* || tar $* || [[ $? -eq 1 ]]
