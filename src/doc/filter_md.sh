#!/usr/bin/env bash

# Math: $expr$ math expressions are well understood by markdown. Turn them into @f$ expr @f$ for doxygen.
math_filter_inline()
{
	sed -e 's/\$/@f$/g'
}

# Block math expressions on their own line. We need to replace the first $$ with @f[ and the second with @f].
# This works only if the whole expression is on one line. Will do for us.
math_filter_block()
{
	sed -e 's/\$\$/@f[/' -e 's/\$\$/@f]/'
}

cat "$1" | math_filter_block | math_filter_inline
