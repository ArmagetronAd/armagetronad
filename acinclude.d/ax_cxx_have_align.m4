# ==================================================================================
#  Adapted from https://www.gnu.org/software/autoconf-archive/ax_cxx_have_bad_function_call.html
# ==================================================================================
#
# SYNOPSIS
#
#   AX_CXX_HAVE_ALIGN()
#
# DESCRIPTION
#
#   This macro checks if std::align, added in C++11, is defined
#   in the <functional> header.
#
#   If it is, define the ax_cv_cxx_have_align environment
#   variable to "yes" and define HAVE_CXX_ALIGN.
#
# LICENSE
#
#   Copyright (c) 2014 Enrico M. Crisostomo <enrico.m.crisostomo@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 3

AC_DEFUN([AX_CXX_HAVE_ALIGN],
  [AC_CACHE_CHECK(
    [for std::align in memory],
    ax_cv_cxx_have_align,
    [dnl
      AC_LANG_PUSH([C++])
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [
          [#include <memory>]
          [using std::align;]
        ],
        []
        )],
        [ax_cv_cxx_have_align=yes],
        [ax_cv_cxx_have_align=no]
      )
    AC_LANG_POP([C++])])
    if test x"$ax_cv_cxx_have_align" = "xyes"
    then
      AC_DEFINE(HAVE_CXX_ALIGN,
        1,
        [Define if memory defines the std::align function.])
    fi
  ])
