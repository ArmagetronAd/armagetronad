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
    rm -f $f
    touch $f
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
    printf "#ifndef %s\n    #define %s \"@%s@\"\n#endif\n" "${variable_u}" "${variable_u}" "${variable}" >> src/tUniversalVariables.h.in.new

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
#clear the file
rm -f batch/relocate.in
touch batch/relocate.in

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

dnl @synopsis AC_LIB_WAD
dnl
dnl This macro searches for installed WAD library.
dnl
AC_DEFUN([AC_LIB_WAD],
[
        AC_REQUIRE([AC_PYTHON_DEVEL])
        AC_ARG_ENABLE(wad,
        AC_HELP_STRING([--enable-wad], [enable wad module]),
        [
                case "${enableval}" in
                        no)     ;;
                        *)      if test "x${enableval}" = xyes;
                                then
                                        check_wad="yes"
                                fi ;;
                esac
        ], [])

        if test -n "$check_wad";
        then
                AC_CHECK_LIB(wadpy, _init, [WADPY=-lwadpy], [], $PYTHON_LDFLAGS $PYTHON_EXTRA_LIBS)
                AC_SUBST(WADPY)
        fi
])

dnl Copyright © 2008 Steven G. Johnson <stevenj@alum.mit.edu>
dnl This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. 
dnl This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. 
dnl You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>. 
dnl As a special exception, the respective Autoconf Macro's copyright owner gives unlimited permission to copy, distribute and modify the configure scripts that are the output of Autoconf when processing the Macro. You need not follow the terms of the GNU General Public License when using or distributing such scripts, even though portions of the text of the Macro appear in them. The GNU General Public License (GPL) does govern all other use of the material that constitutes the Autoconf Macro. 
dnl This special exception to the GPL applies to versions of the Autoconf Macro released by the Autoconf Macro Archive. When you make and distribute a modified version of the Autoconf Macro, you may extend this special exception to the GPL to apply to your modified version as well.

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
	save_LIBS="$LIBS"
	LIBS="$PTHREAD_LIBS $LIBS"
	AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
	AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
	AC_MSG_RESULT($acx_pthread_ok)
	if test x"$acx_pthread_ok" = xno; then
		PTHREAD_LIBS=""
		PTHREAD_CFLAGS=""
	fi
	LIBS="$save_LIBS"
	CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
#      ... -mt is also the pthreads flag for HP/aCC
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
	*solaris*)

	# On Solaris (at least, for some versions), libc contains stubbed
	# (non-functional) versions of the pthreads routines, so link-based
	# tests will erroneously succeed.  (We need to link with -pthreads/-mt/
	# -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
	# a function called by this macro, so we could check for that, but
	# who knows whether they'll stub that too in a future libc.)  So,
	# we'll just look for -pthreads and -lpthread first:

	acx_pthread_flags="-pthreads pthread -mt -pthread $acx_pthread_flags"
	;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

	case $flag in
		none)
		AC_MSG_CHECKING([whether pthreads work without any flags])
		;;

		-*)
		AC_MSG_CHECKING([whether pthreads work with $flag])
		PTHREAD_CFLAGS="$flag"
		;;

		pthread-config)
		AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
		if test x"$acx_pthread_config" = xno; then continue; fi
		PTHREAD_CFLAGS="`pthread-config --cflags`"
		PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
		;;

		*)
		AC_MSG_CHECKING([for the pthreads library -l$flag])
		PTHREAD_LIBS="-l$flag"
		;;
	esac

	save_LIBS="$LIBS"
	save_CFLAGS="$CFLAGS"
	LIBS="$PTHREAD_LIBS $LIBS"
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

	# Check for various functions.  We must include pthread.h,
	# since some functions may be macros.  (On the Sequent, we
	# need a special flag -Kthread to make this header compile.)
	# We check for pthread_join because it is in -lpthread on IRIX
	# while pthread_create is in libc.  We check for pthread_attr_init
	# due to DEC craziness with -lpthreads.  We check for
	# pthread_cleanup_push because it is one of the few pthread
	# functions on Solaris that doesn't have a non-functional libc stub.
	# We try pthread_create on general principles.
	AC_TRY_LINK([#include <pthread.h>],
		    [pthread_t th; pthread_join(th, 0);
		    pthread_attr_init(0); pthread_cleanup_push(0, 0);
		    pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
		    [acx_pthread_ok=yes])

	LIBS="$save_LIBS"
	CFLAGS="$save_CFLAGS"

	AC_MSG_RESULT($acx_pthread_ok)
	if test "x$acx_pthread_ok" = xyes; then
		break;
	fi

	PTHREAD_LIBS=""
	PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
	save_LIBS="$LIBS"
	LIBS="$PTHREAD_LIBS $LIBS"
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

	# Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
	AC_MSG_CHECKING([for joinable pthread attribute])
	attr_name=unknown
	for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
	    AC_TRY_LINK([#include <pthread.h>], [int attr=$attr; return attr;],
			[attr_name=$attr; break])
	done
	AC_MSG_RESULT($attr_name)
	if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
	    AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
			      [Define to necessary symbol if this constant
				uses a non-standard name on your system.])
	fi

	AC_MSG_CHECKING([if more special flags are required for pthreads])
	flag=no
	case "${host_cpu}-${host_os}" in
	    *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
	    *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
	esac
	AC_MSG_RESULT(${flag})
	if test "x$flag" != xno; then
	    PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
	fi

	LIBS="$save_LIBS"
	CFLAGS="$save_CFLAGS"

	# More AIX lossage: must compile with xlc_r or cc_r
	if test x"$GCC" != xyes; then
	  AC_CHECK_PROGS(PTHREAD_CC, xlc_r cc_r, ${CC})
	else
	  PTHREAD_CC=$CC
	fi
else
	PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
	ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
	:
else
	acx_pthread_ok=no
	$2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD


dnl GPLed checks whether the current version of GCC supports a certain flag
dnl source: http://autoconf-archive.cryp.to/ax_cflags_gcc_option.html
dnl Copyright © 2008 Guido U. Draheim <guidod@gmx.de>

AC_DEFUN([AX_CFLAGS_GCC_OPTION_OLD], [dnl
AS_VAR_PUSHDEF([FLAGS],[CFLAGS])dnl
AS_VAR_PUSHDEF([VAR],[ac_cv_cflags_gcc_option_$2])dnl
AC_CACHE_CHECK([m4_ifval($1,$1,FLAGS) for gcc m4_ifval($2,$2,-option)],
VAR,[VAR="no, unknown"
 AC_LANG_SAVE
 AC_LANG_C
 ac_save_[]FLAGS="$[]FLAGS"
for ac_arg dnl
in "-pedantic -Werror % m4_ifval($2,$2,-option)"  dnl   GCC
   "-pedantic % m4_ifval($2,$2,-option) %% no, obsolete"  dnl new GCC
   #
do FLAGS="$ac_save_[]FLAGS "`echo $ac_arg | sed -e 's,%%.*,,' -e 's,%,,'`
   AC_TRY_COMPILE([],[return 0;],
   [VAR=`echo $ac_arg | sed -e 's,.*% *,,'` ; break])
done
 FLAGS="$ac_save_[]FLAGS"
 AC_LANG_RESTORE
])
case ".$VAR" in
     .ok|.ok,*) m4_ifvaln($3,$3) ;;
   .|.no|.no,*) m4_ifvaln($4,$4) ;;
   *) m4_ifvaln($3,$3,[
   if echo " $[]m4_ifval($1,$1,FLAGS) " | grep " $VAR " 2>&1 >/dev/null
   then AC_RUN_LOG([: m4_ifval($1,$1,FLAGS) does contain $VAR])
   else AC_RUN_LOG([: m4_ifval($1,$1,FLAGS)="$m4_ifval($1,$1,FLAGS) $VAR"])
                      m4_ifval($1,$1,FLAGS)="$m4_ifval($1,$1,FLAGS) $VAR"
   fi ]) ;;
esac
AS_VAR_POPDEF([VAR])dnl
AS_VAR_POPDEF([FLAGS])dnl
])


dnl the only difference - the LANG selection... and the default FLAGS

AC_DEFUN([AX_CXXFLAGS_GCC_OPTION_OLD], [dnl
AS_VAR_PUSHDEF([FLAGS],[CXXFLAGS])dnl
AS_VAR_PUSHDEF([VAR],[ac_cv_cxxflags_gcc_option_$2])dnl
AC_CACHE_CHECK([m4_ifval($1,$1,FLAGS) for gcc m4_ifval($2,$2,-option)],
VAR,[VAR="no, unknown"
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 ac_save_[]FLAGS="$[]FLAGS"
for ac_arg dnl
in "-pedantic -Werror % m4_ifval($2,$2,-option)"  dnl   GCC
   "-pedantic % m4_ifval($2,$2,-option) %% no, obsolete"  dnl new GCC
   #
do FLAGS="$ac_save_[]FLAGS "`echo $ac_arg | sed -e 's,%%.*,,' -e 's,%,,'`
   AC_TRY_COMPILE([],[return 0;],
   [VAR=`echo $ac_arg | sed -e 's,.*% *,,'` ; break])
done
 FLAGS="$ac_save_[]FLAGS"
 AC_LANG_RESTORE
])
case ".$VAR" in
     .ok|.ok,*) m4_ifvaln($3,$3) ;;
   .|.no|.no,*) m4_ifvaln($4,$4) ;;
   *) m4_ifvaln($3,$3,[
   if echo " $[]m4_ifval($1,$1,FLAGS) " | grep " $VAR " 2>&1 >/dev/null
   then AC_RUN_LOG([: m4_ifval($1,$1,FLAGS) does contain $VAR])
   else AC_RUN_LOG([: m4_ifval($1,$1,FLAGS)="$m4_ifval($1,$1,FLAGS) $VAR"])
                      m4_ifval($1,$1,FLAGS)="$m4_ifval($1,$1,FLAGS) $VAR"
   fi ]) ;;
esac
AS_VAR_POPDEF([VAR])dnl
AS_VAR_POPDEF([FLAGS])dnl
])

dnl -------------------------------------------------------------------------

AC_DEFUN([AX_CFLAGS_GCC_OPTION_NEW], [dnl
AS_VAR_PUSHDEF([FLAGS],[CFLAGS])dnl
AS_VAR_PUSHDEF([VAR],[ac_cv_cflags_gcc_option_$1])dnl
AC_CACHE_CHECK([m4_ifval($2,$2,FLAGS) for gcc m4_ifval($1,$1,-option)],
VAR,[VAR="no, unknown"
 AC_LANG_SAVE
 AC_LANG_C
 ac_save_[]FLAGS="$[]FLAGS"
for ac_arg dnl
in "-pedantic -Werror % m4_ifval($1,$1,-option)"  dnl   GCC
   "-pedantic % m4_ifval($1,$1,-option) %% no, obsolete"  dnl new GCC
   #
do FLAGS="$ac_save_[]FLAGS "`echo $ac_arg | sed -e 's,%%.*,,' -e 's,%,,'`
   AC_TRY_COMPILE([],[return 0;],
   [VAR=`echo $ac_arg | sed -e 's,.*% *,,'` ; break])
done
 FLAGS="$ac_save_[]FLAGS"
 AC_LANG_RESTORE
])
case ".$VAR" in
     .ok|.ok,*) m4_ifvaln($3,$3) ;;
   .|.no|.no,*) m4_ifvaln($4,$4) ;;
   *) m4_ifvaln($3,$3,[
   if echo " $[]m4_ifval($2,$2,FLAGS) " | grep " $VAR " 2>&1 >/dev/null
   then AC_RUN_LOG([: m4_ifval($2,$2,FLAGS) does contain $VAR])
   else AC_RUN_LOG([: m4_ifval($2,$2,FLAGS)="$m4_ifval($2,$2,FLAGS) $VAR"])
                      m4_ifval($2,$2,FLAGS)="$m4_ifval($2,$2,FLAGS) $VAR"
   fi ]) ;;
esac
AS_VAR_POPDEF([VAR])dnl
AS_VAR_POPDEF([FLAGS])dnl
])


dnl the only difference - the LANG selection... and the default FLAGS

AC_DEFUN([AX_CXXFLAGS_GCC_OPTION_NEW], [dnl
AS_VAR_PUSHDEF([FLAGS],[CXXFLAGS])dnl
AS_VAR_PUSHDEF([VAR],[ac_cv_cxxflags_gcc_option_$1])dnl
AC_CACHE_CHECK([m4_ifval($2,$2,FLAGS) for gcc m4_ifval($1,$1,-option)],
VAR,[VAR="no, unknown"
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 ac_save_[]FLAGS="$[]FLAGS"
for ac_arg dnl
in "-pedantic -Werror % m4_ifval($1,$1,-option)"  dnl   GCC
   "-pedantic % m4_ifval($1,$1,-option) %% no, obsolete"  dnl new GCC
   #
do FLAGS="$ac_save_[]FLAGS "`echo $ac_arg | sed -e 's,%%.*,,' -e 's,%,,'`
   AC_TRY_COMPILE([],[return 0;],
   [VAR=`echo $ac_arg | sed -e 's,.*% *,,'` ; break])
done
 FLAGS="$ac_save_[]FLAGS"
 AC_LANG_RESTORE
])
case ".$VAR" in
     .ok|.ok,*) m4_ifvaln($3,$3) ;;
   .|.no|.no,*) m4_ifvaln($4,$4) ;;
   *) m4_ifvaln($3,$3,[
   if echo " $[]m4_ifval($2,$2,FLAGS) " | grep " $VAR " 2>&1 >/dev/null
   then AC_RUN_LOG([: m4_ifval($2,$2,FLAGS) does contain $VAR])
   else AC_RUN_LOG([: m4_ifval($2,$2,FLAGS)="$m4_ifval($2,$2,FLAGS) $VAR"])
                      m4_ifval($2,$2,FLAGS)="$m4_ifval($2,$2,FLAGS) $VAR"
   fi ]) ;;
esac
AS_VAR_POPDEF([VAR])dnl
AS_VAR_POPDEF([FLAGS])dnl
])

AC_DEFUN([AX_CFLAGS_GCC_OPTION],[ifelse(m4_bregexp([$2],[-]),-1,
[AX_CFLAGS_GCC_OPTION_NEW($@)],[AX_CFLAGS_GCC_OPTION_OLD($@)])])

AC_DEFUN([AX_CXXFLAGS_GCC_OPTION],[ifelse(m4_bregexp([$2],[-]),-1,
[AX_CXXFLAGS_GCC_OPTION_NEW($@)],[AX_CXXFLAGS_GCC_OPTION_OLD($@)])])

dnl include boost boost_thread detection
m4_include(acinclude.d/ax_boost_base.m4)
m4_include(acinclude.d/ax_boost_system.m4)
m4_include(acinclude.d/ax_boost_thread.m4)
