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
AC_DEFUN([AC_AA_PATH],
[
AC_ARG_DIR([$1],[$2],[$3],[$4])
AC_AA_PATH_RAW([$1])
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

dnl @synopsis AX_BOOST([MINIMUM-VERSION])
dnl
dnl Test for the Boost C++ libraries of a particular version (or newer)
dnl
dnl If no path to the installed boost library is given the macro
dnl searchs under /usr, /usr/local, and /opt, and evaluates the
dnl $BOOST_ROOT environment variable. Further documentation is
dnl available at <http://randspringer.de/boost/index.html>.
dnl
dnl This macro calls:
dnl
dnl   AC_SUBST(BOOST_CPPFLAGS) / AC_SUBST(BOOST_LDFLAGS)
dnl
dnl And sets:
dnl
dnl   HAVE_BOOST
dnl
dnl @category InstalledPackages
dnl @category Cxx
dnl @author Thomas Porschberg <thomas@randspringer.de>
dnl @version 2006-06-15
dnl @license AllPermissive

AC_DEFUN([AX_BOOST_BASE],
[
AC_ARG_WITH([boost],
	AS_HELP_STRING([--with-boost@<:@=DIR@:>@], [use boost (default is yes) - it is possible to specify the root directory for boost (optional)]),
	[
    if test "$withval" = "no"; then
		AC_MSG_ERROR([Boost (boost.org) is required for compilation.])
    elif test "$withval" = "yes"; then
        want_boost="yes"
        ac_boost_path=""
    else
	    want_boost="yes"
        ac_boost_path="$withval"
	fi
    ],
    [want_boost="yes"])

if test "x$want_boost" = "xyes"; then
	boost_lib_version_req=ifelse([$1], ,1.20.0,$1)
	boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
	boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
	boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
	boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
	if test "x$boost_lib_version_req_sub_minor" = "x" ; then
		boost_lib_version_req_sub_minor="0"
    	fi
	WANT_BOOST_VERSION=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
	AC_MSG_CHECKING(for boostlib >= $boost_lib_version_req)
	succeeded=no

	dnl first we check the system location for boost libraries
	dnl this location ist chosen if boost libraries are installed with the --layout=system option
	dnl or if you install boost with RPM
	if test "$ac_boost_path" != ""; then
		BOOST_LDFLAGS="-L$ac_boost_path/lib"
		BOOST_CPPFLAGS="-I$ac_boost_path/include"
	else
		for ac_boost_path_tmp in /usr /usr/local /opt ; do
			if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
				BOOST_LDFLAGS="-L$ac_boost_path_tmp/lib"
				BOOST_CPPFLAGS="-I$ac_boost_path_tmp/include"
				break;
			fi
		done
	fi

	CPPFLAGS_SAVED="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
	export CPPFLAGS

	LDFLAGS_SAVED="$LDFLAGS"
	LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
	export LDFLAGS

	AC_LANG_PUSH(C++)
     	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	@%:@include <boost/version.hpp>
	]], [[
	#if BOOST_VERSION >= $WANT_BOOST_VERSION
	// Everything is okay
	#else
	#  error Boost version is too old
	#endif
	]])],[
        AC_MSG_RESULT(yes)
	succeeded=yes
	found_system=yes
       	],[
       	])
	AC_LANG_POP([C++])



	dnl if we found no boost with system layout we search for boost libraries
	dnl built and installed without the --layout=system option or for a staged(not installed) version
	if test "x$succeeded" != "xyes"; then
		_version=0
		if test "$ac_boost_path" != ""; then
               		BOOST_LDFLAGS="-L$ac_boost_path/lib"
			if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
				for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
					_version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
					V_CHECK=`expr $_version_tmp \> $_version`
					if test "$V_CHECK" = "1" ; then
						_version=$_version_tmp
					fi
					VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
					BOOST_CPPFLAGS="-I$ac_boost_path/include/boost-$VERSION_UNDERSCORE"
				done
			fi
		else
			for ac_boost_path in /usr /usr/local /opt ; do
				if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
					for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
						_version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
						V_CHECK=`expr $_version_tmp \> $_version`
						if test "$V_CHECK" = "1" ; then
							_version=$_version_tmp
	               					best_path=$ac_boost_path
						fi
					done
				fi
			done

			VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
			BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
			BOOST_LDFLAGS="-L$best_path/lib"

	    		if test "x$BOOST_ROOT" != "x"; then
				if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
					version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
					stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
			        	stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
					V_CHECK=`expr $stage_version_shorten \>\= $_version`
				        if test "$V_CHECK" = "1" ; then
						AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
						BOOST_CPPFLAGS="-I$BOOST_ROOT"
						BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
					fi
				fi
	    		fi
		fi

		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
		export CPPFLAGS
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
		export LDFLAGS

		AC_LANG_PUSH(C++)
	     	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		@%:@include <boost/version.hpp>
		]], [[
		#if BOOST_VERSION >= $WANT_BOOST_VERSION
		// Everything is okay
		#else
		#  error Boost version is too old
		#endif
		]])],[
        	AC_MSG_RESULT(yes)
		succeeded=yes
		found_system=yes
       		],[
	       	])
		AC_LANG_POP([C++])
	fi

	if test "$succeeded" != "yes" ; then
		if test "$_version" = "0" ; then
			AC_MSG_ERROR([[We could not detect the boost libraries (version $boost_lib_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
		else
			AC_MSG_NOTICE([Your boost libraries seems to old (version $_version).])
		fi
	else
		AC_SUBST(BOOST_CPPFLAGS)
		AC_SUBST(BOOST_LDFLAGS)
		AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])
	fi

        CPPFLAGS="$CPPFLAGS_SAVED"
       	LDFLAGS="$LDFLAGS_SAVED"
fi

])

dnl @synopsis AC_PROG_SWIG([major.minor.micro])
dnl
dnl This macro searches for a SWIG installation on your system. If
dnl found you should call SWIG via $(SWIG). You can use the optional
dnl first argument to check if the version of the available SWIG is
dnl greater than or equal to the value of the argument. It should have
dnl the format: N[.N[.N]] (N is a number between 0 and 999. Only the
dnl first N is mandatory.)
dnl
dnl If the version argument is given (e.g. 1.3.17), AC_PROG_SWIG checks
dnl that the swig package is this version number or higher.
dnl
dnl In configure.in, use as:
dnl
dnl             AC_PROG_SWIG(1.3.17)
dnl             SWIG_ENABLE_CXX
dnl             SWIG_MULTI_MODULE_SUPPORT
dnl             SWIG_PYTHON
dnl
dnl @category InstalledPackages
dnl @author Sebastian Huber <sebastian-huber@web.de>
dnl @author Alan W. Irwin <irwin@beluga.phys.uvic.ca>
dnl @author Rafael Laboissiere <rafael@laboissiere.net>
dnl @author Andrew Collier <abcollier@yahoo.com>
dnl @version 2004-09-20
dnl @license GPLWithACException

AC_DEFUN([AC_PROG_SWIG],[
        AC_PATH_PROG([SWIG],[swig])
        if test -z "$SWIG" ; then
                AC_MSG_WARN([cannot find 'swig' program. You should look at http://www.swig.org])
                SWIG='echo "Error: SWIG is not installed. You should look at http://www.swig.org" ; false'
        elif test -n "$1" ; then
                AC_MSG_CHECKING([for SWIG version])
                [swig_version=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
                AC_MSG_RESULT([$swig_version])
                if test -n "$swig_version" ; then
                        # Calculate the required version number components
                        [required=$1]
                        [required_major=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_major" ; then
                                [required_major=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_minor=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_minor" ; then
                                [required_minor=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_patch=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_patch" ; then
                                [required_patch=0]
                        fi
                        # Calculate the available version number components
                        [available=$swig_version]
                        [available_major=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_major" ; then
                                [available_major=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_minor=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_minor" ; then
                                [available_minor=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_patch=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_patch" ; then
                                [available_patch=0]
                        fi
                        if test $available_major -ne $required_major \
                                -o $available_minor -ne $required_minor \
                                -o $available_patch -lt $required_patch ; then
                                AC_MSG_WARN([SWIG version >= $1 is required.  You have $swig_version.  You should look at http://www.swig.org])
                                SWIG='echo "Error: SWIG version >= $1 is required.  You have '"$swig_version"'.  You should look at http://www.swig.org" ; false'
                        else
                                AC_MSG_NOTICE([SWIG executable is '$SWIG'])
                                SWIG_LIB=`$SWIG -swiglib`
                                AC_MSG_NOTICE([SWIG library directory is '$SWIG_LIB'])
                        fi
                else
                        AC_MSG_WARN([cannot determine SWIG version])
                        SWIG='echo "Error: Cannot determine SWIG version.  You should look at http://www.swig.org" ; false'
                fi
        fi
        AC_SUBST([SWIG_LIB])
])

# SWIG_ENABLE_CXX()
#
# Enable SWIG C++ support.  This affects all invocations of $(SWIG).
AC_DEFUN([SWIG_ENABLE_CXX],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PROG_CXX])
        SWIG="$SWIG -c++"
])

# SWIG_MULTI_MODULE_SUPPORT()
#
# Enable support for multiple modules.  This effects all invocations
# of $(SWIG).  You have to link all generated modules against the
# appropriate SWIG runtime library.  If you want to build Python
# modules for example, use the SWIG_PYTHON() macro and link the
# modules against $(SWIG_PYTHON_LIBS).
#
AC_DEFUN([SWIG_MULTI_MODULE_SUPPORT],[
        AC_REQUIRE([AC_PROG_SWIG])
        SWIG="$SWIG -noruntime"
])

# SWIG_PYTHON([use-shadow-classes = {no, yes}])
#
# Checks for Python and provides the $(SWIG_PYTHON_CPPFLAGS),
# and $(SWIG_PYTHON_OPT) output variables.
#
# $(SWIG_PYTHON_OPT) contains all necessary SWIG options to generate
# code for Python.  Shadow classes are enabled unless the value of the
# optional first argument is exactly 'no'.  If you need multi module
# support (provided by the SWIG_MULTI_MODULE_SUPPORT() macro) use
# $(SWIG_PYTHON_LIBS) to link against the appropriate library.  It
# contains the SWIG Python runtime library that is needed by the type
# check system for example.
AC_DEFUN([SWIG_PYTHON],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PYTHON_DEVEL])
        test "x$1" != "xno" || swig_shadow=" -noproxy"
        AC_SUBST([SWIG_PYTHON_OPT],[-python$swig_shadow])
        AC_SUBST([SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])
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
