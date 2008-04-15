# Check whether make supports sinclude
# Manuel Moos, 2005
AC_DEFUN([AC_SINCLUDE],
[
    AC_MSG_CHECKING(whether ${MAKE-make} understands [sinc[]lude])
    echo "sinc[]lude" mf2 > mf1
    echo "test:;" > mf2

    if ${MAKE-make} -f mf1 test > /dev/null 2>&1; then
        silent_inc=sin[]clude
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
    	AC_MSG_CHECKING(whether ${MAKE-make} understands -include)
    	echo "-incl[]ude" mf2 > mf1
    	echo "test:;" > mf2

    	if ${MAKE-make} -f mf1 test > /dev/null 2>&1; then
    	    silent_inc=-inc[]lude
    	    AC_MSG_RESULT(yes)
    	else
    	   silent_inc=inc[]lude
           AC_MSG_RESULT(no)
        fi
    fi

    AC_SUBST(silent_inc)
    rm -f mf1 mf2
]
)

# helper for AC_SUBST_UNIVERSAL, does not call AC_SUBST.
# useful to enhance regular autoconf variables (like @prefix@)
# to universal variables.

AC_DEFUN([AC_SUBST_UNIVERSAL_RAW],
[
ac_aa_universal_variables="${ac_aa_universal_variables} $1"
export [$1]
])

# Adds a universal variable. If the argument is foobar, then
# you'll get a @foobar@ replacement for the batch files and
# other .in files, a foobar variable in all makefiles and a 
# FOOBAR preprocessor macro in C files that include 
# tUniversalVariables.h.
# The value is always the value of the shell variable foobar
# at the time of the AC_OUTPUT call.

AC_DEFUN([AC_SUBST_UNIVERSAL],
[
AC_SUBST_UNIVERSAL_RAW([$1])
AC_SUBST([$1])
]
)

# applies the registered path variables. Needs to be called after
# all AC_SUBST_UNIVERSAL calls and before AC_OUTPUT.
AC_DEFUN([AC_USE_SUBST_UNIVERSAL],
[
#echo uvs=${ac_aa_universal_variables}
# put the path names into a file
AC_CONFIG_COMMANDS(universal_variables,[
# clear files
for f in src/tUniversalVariables.h.in.new universal_variables universal_variable_substitutions universal_variable_values.in universal_variable_values_makefile.new universal_variable_values_makefile_sed.new; do
    echo -n "" > $f
done

# prepare value inserting makefile:
# the part reading
# ${top_builddir}/universal_variable_values: ${top_builddir}/universal_variable_values_makefile Makefile
#	@echo Generating universal_variable_values...

# it generates the file universal_variable_values, a shell script with our
# universal variables set. It's currently unused.

echo \${top_builddir}/universal_variable_values: \${top_builddir}/universal_variable_values_makefile Makefile > universal_variable_values_makefile.new
echo "	@echo Generating universal_variable_values..." >> universal_variable_values_makefile.new
echo "	@echo \# values of universal variables at compile time > \${top_builddir}/universal_variable_values" >> universal_variable_values_makefile.new

# ghe part reading
#${top_builddir}/universal_variable_values_sed: ${top_builddir}/universal_variable_values_makefile Makefile
#	@echo Generating universal_variable_values_sed...

# it gnerates the file universal_variable_values_sed, a sed script replacing
# @foo@ with the build time value of ${foo} for every universal varialbe
# foo.

echo \${top_builddir}/universal_variable_values_sed: \${top_builddir}/universal_variable_values_makefile Makefile > universal_variable_values_makefile_sed.new
echo "	@echo Generating universal_variable_values_sed..." >> universal_variable_values_makefile_sed.new
echo "	@echo \# unversal variable values at compile time > \${top_builddir}/universal_variable_values_sed" >> universal_variable_values_makefile_sed.new

for variable in ${ac_aa_universal_variables}; do
    # echo Adding universal variable ${variable}...
    
    # make variable uppercase (do it the hard way, some systems don't have \U)
    variable_u=`echo ${variable} | sed -e 's/a/A/g' -e 's/b/B/g' -e 's/c/C/g' -e 's/d/D/g' -e 's/e/E/g' -e 's/f/F/g' -e 's/g/G/g' -e 's/h/H/g' -e 's/i/I/g' -e 's/j/J/g' -e 's/k/K/g' -e 's/l/L/g' -e 's/m/M/g' -e 's/n/N/g' -e 's/o/O/g' -e 's/p/P/g' -e 's/q/Q/g' -e 's/r/R/g' -e 's/s/S/g' -e 's/t/T/g' -e 's/u/U/g' -e 's/v/V/g' -e 's/w/W/g' -e 's/x/X/g' -e 's/y/Y/g' -e 's/z/Z/g'`

    # add C macro to prototype file (later modified by the makefile
    # to contain the possibly modified variable values)
    echo -e "#ifndef ${variable_u}\n    #define ${variable_u} \"@${variable}@\"\n#endif" >> src/tUniversalVariables.h.in.new

    # generate prototype for file containing the actual values of the variables
    # echo ${variable}=@${variable}@  >> universal_variable_values.in

    # generate makefile for file containing the variable values at compile time
    # shell script part
    echo "	@echo ${variable}=\${${variable}} >> \${top_builddir}/universal_variable_values" >> universal_variable_values_makefile.new
    # sed script part
    echo "	@echo s,@${variable}@,\${${variable}},g >> \${top_builddir}/universal_variable_values_sed" >> universal_variable_values_makefile_sed.new

    # generate file with substitutions @foobar@ -> ${foobar}
    echo s,@${variable}@,\$\{${variable}\},g >> universal_variable_substitutions
done

# concatenate the two makefiles to one
echo "" >> universal_variable_values_makefile.new
cat universal_variable_values_makefile_sed.new >> universal_variable_values_makefile.new
rm -f universal_variable_values_makefile_sed.new

# replace files if they were changed changed
for f in src/tUniversalVariables.h.in universal_variable_values_makefile; do
    diff $f $f.new > /dev/null 2>&1 || echo "updating $f"; mv $f.new $f
    rm -f $f.new
done

# make a list of all universal variables (currently unused)
echo ${ac_aa_universal_variables} > universal_variables
]
,
ac_aa_universal_variables="${ac_aa_universal_variables}")
])

# Adds a directory configuration item to be configured via configure,
# make variables or environment variables while the game is run.
# argument: the name of the path variable.
AC_DEFUN([AC_AA_PATH_NOSUFFIX_RAW],
[
ac_aa_pathvars="${ac_aa_pathvars} $1"
AC_SUBST_UNIVERSAL_RAW($1)
])

# Adds a directory configuration item to be configured via configure,
# make variables or environment variables while the game is run.
# argument: the name of the path variable.
# If you add foodir, you also get foodir_suffix and aa_foodir.
# foodir_suffix is what we append to foodir (usually /armagetronad or /games/armagetronad),
# and aa_foodir is the complete composed path.
AC_DEFUN([AC_AA_PATH_RAW],
[
AC_AA_PATH_NOSUFFIX_RAW($1)
AC_SUBST_UNIVERSAL($1_suffix)
AC_SUBST_UNIVERSAL(aa_$1)
aa_$1=\${$1}\${$1_suffix}\${progdir_suffix}
])

# Fallback function if accustomdir is not included
# Arguments:
# 1. the name of the directory configuration variable
# 2. the default value
# 3. a description of the purpose of the directory
# 4. human-form of the default value
AC_DEFUN([AC_ARG_DIR],
[
AC_ARG_WITH([$1],AC_HELP_STRING([--with-$1=DIR],[directory used for $3 (default: $4)]),[$1=${enableval}],[$1=$2
$1_enabled=no])
]
)

# Adds a directory configuration item to be configured via configure,
# make variables or environment variables while the game is run.
# also adds a command line argument to --configure.
# Arguments:
# 1. the name of the directory configuration variable
# 2. the default value
# 3. a description of the purpose of the directory
# 4. human-form of the default value
AC_DEFUN([AC_AA_PATHOPT],
[
AC_ARG_DIR([$1],[$2],[$3],[$4])
AC_SUBST([$1])
]
)
AC_DEFUN([AC_AA_PATH_WITH],
[
AC_ARG_WITH([$1],AC_HELP_STRING([--with-$1=DIR],[$3 (default: $1)]),[$1=${enableval}],[$1=$2
$1_enabled=no])
AC_AA_PATH_RAW([$1])
AC_SUBST([$1])
]
)

AC_DEFUN([AC_AA_PATH_NOSUFFIX],
[
AC_AA_PATHOPT([$1],[$2],[$3],[$4])
AC_AA_PATH_NOSUFFIX_RAW([$1])
]
)
AC_DEFUN([AC_AA_PATH],
[
AC_AA_PATHOPT([$1],[$2],[$3],[$4])
AC_AA_PATH_RAW([$1])
]
)

# applies the registered path variables 
AC_DEFUN([AC_AA_REPLACEPATHS],
[
#echo ap=${ac_aa_pathvars}
# put the path names into a file
AC_CONFIG_COMMANDS(pathsubstitution,[
echo -n "" > batch/relocate.in

# generate path relocation sed script: replaces @foo_reloc@ with the
# output of `relocate @foo@`, where @foo@ itself is replaced during
# build time.
# the resulting script is then embedded into sysinstall where it is
# called at installation time and processes all scripts.
for path in ${ac_aa_pathvars}; do
    #echo Adding path ${path}...
    for variable in ${path} aa_${path}; do
        echo "                -e \\\"s,@${variable}_reloc@,\`relocate_root @${variable}@\`,g\\\"\\" >> batch/relocate.in
    done
done

# generate list
echo ${ac_aa_pathvars} > extrapaths
]
,
ac_aa_pathvars="${ac_aa_pathvars}")
])

# prepares a standard AA path: defines a suitable suffix (containing /games
# if the original path doesn't do so already and /armagetronad)
AC_DEFUN([AC_AA_PATH_PREPARE],
[
# default suffix: /armagetronad(-dedicated)
test -z "${$1_suffix}" && $1_suffix=/${progname}
if test "x${enable_games}" = "xyes"; then
    echo "${$1}${$1_suffix}" | grep "/games" > /dev/null || $1_suffix=/games${$1_suffix}
fi
])

# like AC_AA_PATH_PREPARE, but also checks if the path value is part of a
# prefix or not; if not, binreloc is disabled. The second argument is the
# prefix to expect.
AC_DEFUN([AC_AA_PATH_PREPARE_BINRELOC],
[
AC_AA_PATH_PREPARE($1)
test -z "${binreloc_auto}" && binreloc_auto=auto
if test "x${binreloc_auto}" = "xauto" && echo ${$1} | grep -v "^$2" > /dev/null; then
    AC_MSG_WARN([Path $1(=${$1}) does not begin with the right prefix(=$2),
disabling relocation support. You can try to enforce it with --enable-binreloc,
but you are on your own then.])
    binreloc_auto=no
fi
])

# Check for binary relocation support
# Hongli Lai
# http://autopackage.org/

# MODIFICATION FOR AA: changed enable_binreloc_=auto to enable_binreloc=${binreloc_auto} so we can automatically disable it

AC_DEFUN([AM_BINRELOC],
[
	AC_ARG_ENABLE(binreloc,
		[  --enable-binreloc       compile with binary relocation support
                          (default=enable when available)],
		enable_binreloc=$enableval,enable_binreloc=${binreloc_auto})

	AC_ARG_ENABLE(binreloc-threads,
		[  --enable-binreloc-threads      compile binary relocation with threads support
	                         (default=yes)],
		enable_binreloc_threads=$enableval,enable_binreloc_threads=yes)

	BINRELOC_CFLAGS=
	BINRELOC_LIBS=
	if test "x$enable_binreloc" = "xauto"; then
		AC_CHECK_FILE([/proc/self/maps])
		AC_CACHE_CHECK([whether everything is installed to the same prefix],
			       [br_cv_valid_prefixes], [
				if test "$bindir" = '${exec_prefix}/bin' -a "$sbindir" = '${exec_prefix}/sbin' -a \
					"$datadir" = '${prefix}/share' -a "$libdir" = '${exec_prefix}/lib' -a \
					"$libexecdir" = '${exec_prefix}/libexec' -a "$sysconfdir" = '${prefix}/etc'
				then
					br_cv_valid_prefixes=yes
				else
					br_cv_valid_prefixes=no
				fi
				])
	fi
	AC_CACHE_CHECK([whether binary relocation support should be enabled],
		       [br_cv_binreloc],
		       [if test "x$enable_binreloc" = "xyes"; then
		       	       br_cv_binreloc=yes
		       elif test "x$enable_binreloc" = "xauto"; then
			       if test dnl "x$br_cv_valid_prefixes" = "xyes" -a \
			       	       "x$ac_cv_file__proc_self_maps" = "xyes"; then
				       br_cv_binreloc=yes
			       else
				       br_cv_binreloc=no
			       fi
		       else
			       br_cv_binreloc=no
		       fi])

	if test "x$br_cv_binreloc" = "xyes"; then
		BINRELOC_CFLAGS="-DENABLE_BINRELOC"
		AC_DEFINE(ENABLE_BINRELOC,,[Use binary relocation?])
		if test "x$enable_binreloc_threads" = "xyes"; then
			AC_CHECK_LIB([pthread], [pthread_getspecific])
		fi

		AC_CACHE_CHECK([whether binary relocation should use threads],
			       [br_cv_binreloc_threads],
			       [if test "x$enable_binreloc_threads" = "xyes"; then
					if test "x$ac_cv_lib_pthread_pthread_getspecific" = "xyes"; then
						br_cv_binreloc_threads=yes
					else
						br_cv_binreloc_threads=no
					fi
			        else
					br_cv_binreloc_threads=no
				fi])

		if test "x$br_cv_binreloc_threads" = "xyes"; then
			BINRELOC_LIBS="-lpthread"
			AC_DEFINE(BR_PTHREAD,1,[Include pthread support for binary relocation?])
		else
			BINRELOC_CFLAGS="$BINRELOC_CFLAGS -DBR_PTHREADS=0"
			AC_DEFINE(BR_PTHREAD,0,[Include pthread support for binary relocation?])
		fi
	fi
	AC_SUBST(BINRELOC_CFLAGS)
	AC_SUBST(BINRELOC_LIBS)
])

dnl Modified ZThread test follows. This one works, the official one alone does not.
dnl Changes: hacked compiler temporarily to use CXX instead of CC
dnl          added zthread/ to include path
dnl          added #include "zthread/Task.h" and #include "zthread/Thread.h"
dnl          added HAVE_LIBZTHREAD define

dnl Copyright (c) 2005, Eric Crahen
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a copy
dnl of this software and associated documentation files (the "Software"), to deal
dnl in the Software without restriction, including without limitation the rights
dnl to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
dnl copies of the Software, and to permit persons to whom the Software is furnished
dnl to do so, subject to the following conditions:
dnl 
dnl The above copyright notice and this permission notice shall be included in all
dnl copies or substantial portions of the Software.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
dnl AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
dnl WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
dnl CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

dnl Detect the library and include paths for ZThreads, perform some test
dnl compilations.
dnl
dnl Should be used in AC_PROG_CC mode before the swtich to C++ if any is made
dnl (eg before AC_LANG_CPLUSPLUS)
dnl
dnl --with-zthread-prefix : Skip detection, use this general path
dnl --with-zthread-exec-prefix : Skip detecting the zthread-config tool
dnl 
dnl Sets the following variables.
dnl
dnl ZTHREAD_CXXFLAGS
dnl ZTHREAD_LIBS
dnl 
AC_DEFUN([AM_PATH_ZTHREAD_AA],
[


AC_ARG_WITH(zthread-prefix,[  --with-zthread-prefix=PFX   Prefix where ZTHREAD is installed (optional)],
            zthread_prefix="$withval", zthread_prefix="")
AC_ARG_WITH(zthread-exec-prefix,[  --with-zthread-exec-prefix=PFX Exec prefix where ZTHREAD is installed (optional)],
            zthread_exec_prefix="$withval", zthread_exec_prefix="")
AC_ARG_ENABLE(zthreadtest, [  --disable-zthreadtest       Do not try to compile and run a test ZTHREAD program],
		    , enable_zthreadtest=yes)

  if test x$zthread_exec_prefix != x ; then
     zthread_args="$zthread_args --exec-prefix=$zthread_exec_prefix"
     if test x${ZTHREAD_CONFIG+set} != xset ; then
        ZTHREAD_CONFIG=$zthread_exec_prefix/bin/zthread-config
     fi
  fi
  if test x$zthread_prefix != x ; then
     zthread_args="$zthread_args --prefix=$zthread_prefix"
     if test x${ZTHREAD_CONFIG+set} != xset ; then
        ZTHREAD_CONFIG=$zthread_prefix/bin/zthread-config
     fi
  fi

  AC_PATH_PROG(ZTHREAD_CONFIG, zthread-config, no)
  min_zthread_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for ZTHREAD - version >= $min_zthread_version)
  no_zthread=""
  if test "$ZTHREAD_CONFIG" = "no" ; then
    no_zthread=yes
  else
    ZTHREAD_CXXFLAGS=`$ZTHREAD_CONFIG $zthreadconf_args --cflags`
    ZTHREAD_LIBS=`$ZTHREAD_CONFIG $zthreadconf_args --libs`

    zthread_major_version=`$ZTHREAD_CONFIG $zthread_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    zthread_minor_version=`$ZTHREAD_CONFIG $zthread_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    zthread_micro_version=`$ZTHREAD_CONFIG $zthread_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_zthreadtest" = "xyes" ; then
      ac_save_CXXFLAGS="$CXXFLAGS"
      ac_save_CFLAGS="$CXXFLAGS"
      ac_save_LIBS="$LIBS"
      CXXFLAGS="$CXXFLAGS $ZTHREAD_CXXFLAGS"
      CFLAGS="$CFLAGS $ZTHREAD_CXXFLAGS"
      LIBS="$LIBS $ZTHREAD_LIBS"

dnl
dnl Now check if the installed ZTHREAD is sufficiently new. (Also sanity
dnl checks the results of zthread-config to some extent
dnl
      rm -f conf.zthreadtest
      CC_OLD=${CC}
      CC=${CXX}
      for extra_flags in "NONE" "-fpermissive"; do
        if test NONE != ${extra_flags}; then
          ZTHREAD_CXXFLAGS="$ZTHREAD_CXXFLAGS ${extra_flags}"
          CXXFLAGS="$CXXFLAGS ${extra_flags}"
          CFLAGS="$CFLAGS ${extra_flags}"
        fi
        if test "x$no_zthread" = xyes || test NONE = ${extra_flags} ; then
        no_zthread=""
        AC_TRY_RUN([


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "zthread/Task.h"
#include "zthread/Thread.h"
#include "zthread/ZThread.h"

int main (int argc, char *argv[]) {

  int major, minor, micro;
  char tmp_version[256];

  { FILE *fp = fopen("conf.zthreadtest", "a"); if ( fp ) fclose(fp); }


  /* HP/UX 9 (%@#!) writes to sscanf strings */
  strcpy(tmp_version, "$min_zthread_version");
  
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {

    printf("%s, bad version string\n", "$min_zthread_version");
    return 1;

  }

  if (($zthread_major_version > major) ||
     (($zthread_major_version == major) && ($zthread_minor_version > minor)) ||
     (($zthread_major_version == major) && ($zthread_minor_version == minor) && 
     ($zthread_micro_version >= micro))) {
      return 0;
  } else {

    printf("\n*** 'zthread-config --version' returned %d.%d.%d, but the minimum version\n", $zthread_major_version, $zthread_minor_version, $zthread_micro_version);
    printf("*** of ZThread required is %d.%d.%d. If zthread-config is correct, then it is\n", major, minor, micro);
    printf("*** best to upgrade to the required version.\n");
    printf("*** If zthread-config was wrong, set the environment variable ZTHREAD_CONFIG\n");
    printf("*** to point to the correct copy of zthread-config, and remove the file\n");
    printf("*** config.cache before re-running configure\n");

    return 1;
  }

}

],, no_zthread=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
      fi
    done
    CC=${CC_OLD}
    CXXFLAGS="$ac_save_CXXFLAGS"
    LIBS="$ac_save_LIBS"
    fi
  fi

  if test "x$no_zthread" = x ; then
    
     AC_DEFINE(HAVE_LIBZTHREAD,[], [Define if you have the library ZThread.])
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     

  else

     AC_MSG_RESULT(no)

     if test "$ZTHREAD_CONFIG" = "no" ; then

       echo "*** The zthread-config script installed by ZThread could not be found"
       echo "*** If ZThread was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the ZTHREAD_CONFIG environment variable to the"
       echo "*** full path to zthread-config."

     else
       if test -f conf.zthreadtest ; then
        :
       else

          echo "*** Could not run ZThread test program, checking why..."
          CXXFLAGS="$CXXFLAGS $ZTHREAD_CXXFLAGS"
          LIBS="$LIBS $ZTHREAD_LIBS"

          echo $LIBS;

          AC_TRY_LINK([#include "zthread/ZThread.h"], 
                      [ return 0; ], [
          echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding ZThread or finding the wrong"
          echo "*** version of ZThread. If it is not finding ZThread, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	        echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means ZThread was incorrectly installed"
          echo "*** or that you have moved ZThread since it was installed. In the latter case, you"
          echo "*** may want to edit the zthread-config script: $ZTHREAD_CONFIG" ])
          CFLAGS="$ac_save_CXXFLAGS"
          CXXFLAGS="$ac_save_CXXFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi

     ZTHREAD_CXXFLAGS=""
     ZTHREAD_LIBS=""
     ifelse([$3], , :, [$3])

  fi

  AC_SUBST(ZTHREAD_CXXFLAGS)
  AC_SUBST(ZTHREAD_LIBS)

  rm -f conf.zthreadtest

])
