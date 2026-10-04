#!/bin/sh

# perform a sensible git clang-format

# usage: 

# ./batch/git_clang_format.sh

# to apply format before committing

# ./batch/git_clang_format.sh <base branch name>

# to reformat all changes that are new to your feature/bugfix branch

git clang-format --extensions cpp,h "$@"
