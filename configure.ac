dnl I would like to do this to generate the version number...
dnl AC_DEFUN([AUTOMATIC_VERSION],syscmd([sh batch/make/version .]))

dnl but have to do this with version generated in bootstrap.sh instead for some reason
m4_include(version)

dnl Comment this out to disable autoconf custom directories:
m4_include(accustomdir.m4)

dnl Process this file with autoconf to produce a configure script.
AC_INIT(armagetronad,AUTOMATIC_VERSION)

dnl FIXME: remove this in the future, when automake decides to actually set datarootdir by default
dnl http://www.gnu.org/software/hello/manual/autoconf/Changed-Directory-Variables.html
AC_DEFUN([AC_DATAROOTDIR_CHECKED])

#set
#echo args : "$@"
#exit 1

dnl flag telling the status message whether some files will be installed outside of prefix
OUTSIDE_OF_PREFIX=no

dnl system type
AC_CANONICAL_TARGET

dnl those two are apparently included in AC_CANONICAL_TARGET
dnl AC_CANONICAL_BUILD
dnl AC_CANONICAL_HOST

AC_PROG_RANLIB

dnl use slighly less ancient tar format allowing slighlty longer filenames.
AM_INIT_AUTOMAKE([tar-ustar filename-length-max=255])

dnl detect which link commant to use
AC_PROG_LN_S

dnl find a parser generator to use
AC_PROG_YACC

dnl check whether m4 is available for documentation processing
AC_CHECK_PROG(HAVE_M4,m4,true,false)
AM_CONDITIONAL(HAVE_M4, test $HAVE_M4 = true)

dnl check whether date accepts -r to refer to a file
AM_CONDITIONAL(DATE_R, date +"%Y" -r $0 > /dev/null 2>&1)

dnl export version
version=$VERSION
AC_SUBST(version)

AC_ARG_VAR(XML2_CONFIG,[Configuration script of libxml2 to use, defaults to xml2-config])
AC_ARG_VAR(SDL_CONFIG,[Configuration script of SDL to use, defaults to sdl-config])
AC_ARG_VAR(FREETYPE_CONFIG,[Configuration script of freetype to use, defaults to freetype-config])
AC_ARG_VAR(CXXFLAGS,Flags passed to the C++ compiler)
AC_ARG_VAR(DEBUGLEVEL,[Debug level for developers, normal users can safely ignore it. Accepted values go from 0 to 5. See README-DEVELOPER for details.])
AC_ARG_VAR(CODELEVEL,[Code checking strictness level for developers, normal users can safely ignore it. Accepted values go from 0 to 4. See README-DEVELOPER for details.])
AC_ARG_VAR(docstyle,[Documentation style for cross-building of documentation. Possible values: unix, web and windows.])
AC_ARG_VAR(progname,[Program short name, default: armagetronad.])
AC_ARG_VAR(progtitle,[Program long name, default: Armagetron Advanced.])
AC_ARG_VAR(APBUILD_CXX1,[Autopackage ABI 1 C++ compiler.])
AC_ARG_VAR(APBUILD_CXX2,[Autopackage ABI 2 C++ compiler.])
AC_ARG_VAR(APBUILD_CC,[Autopackage C compiler.])
AC_ARG_VAR(CXX_ABI,[Autopackage C++ ABI version.])
AC_ARG_VAR(APBUILD_STATIC,[Autopackage static libraries.])

dnl a modern autoconf is required to process this
AC_PREREQ(2.50)

build_regular=true

have_lzma=false
    AC_ARG_WITH(maxclients, AC_HELP_STRING([--with-maxclients],[sets the hardcoded limit for the number of connected clients (default: 16 for clients and 32 for dedicated servers)]),
[ 
    AC_DEFINE_UNQUOTED(MAXCLIENTS,${withval},maximal number of clients)
])

AC_ARG_ENABLE(dedicated,
     AC_HELP_STRING([--enable-dedicated],
		[enables dedicated server; otherwise, compile a client (default=disabled)]),
     [case "${enableval}" in
       yes) build_dedicated=true
			build_regular=false
	   	;;
       no)  build_dedicated=false
			build_regular=true
	    ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-dedicated) ;;
     esac],[build_dedicated=false])


dnl AC_ARG_ENABLE(glout,
dnl [  --enable-glout          enable graphical output; otherwise, compile
dnl                          a dedicated server (default=enabled)],,
dnl enable_glout=yes)

AC_ARG_ENABLE(master,
     AC_HELP_STRING([--enable-master],
		[Build the master server]),
     [case "${enableval}" in
       yes) armamaster=true ;;
       no)  armamaster=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-master) ;;
     esac],[armamaster=false])

AM_CONDITIONAL(BUILDMASTER, test x$armamaster = xtrue)

AC_ARG_ENABLE(main,
     AC_HELP_STRING([--enable-main],
		[Build the main program]),
     [case "${enableval}" in
       yes) armamain=true ;;
       no)  armamain=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-main) ;;
     esac],[armamain=true])

AC_ARG_ENABLE(glout,
     AC_HELP_STRING([--enable-glout],
		[Inverse alias of --enable-dedicated]),
     [case "${enableval}" in
       yes) build_dedicated=false
			build_regular=true
	   	;;
       no)  build_dedicated=true
			build_regular=false
	    ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-glout) ;;
     esac],)

# consistency check of the three build flags	 
test x$armamain = xfalse && build_dedicated=false build_regular=false
test x$build_dedicated = xfalse && test x$build_regular = xfalse && armamain=false

AM_CONDITIONAL(BUILDDEDICATED, test x$build_dedicated = xtrue)
AM_CONDITIONAL(BUILDREGULAR, test x$build_regular = xtrue)
AM_CONDITIONAL(BUILDMAIN, test x$armamain = xtrue)

# flag to set when mockup versions of the executables should be built to save
# compile time when testing the release process
AM_CONDITIONAL(BUILDFAKERELEASE, test "$ARMAGETRONAD_FAKERELEASE" != "")

# set replacement variables for dedicated server
progtitle_serverorclient=" Client"
progtitle_server=""
if test x$build_dedicated = xtrue; then
   progtitle_serverorclient=" Server"
   progtitle_server=" Server"
fi
AC_SUBST(progtitle_serverorclient)
AC_SUBST(progtitle_server)
AC_SUBST(build_dedicated)
AC_SUBST(build_regular)

AC_ARG_ENABLE(memmanager,
	AC_HELP_STRING([--enable-memmanager],
		[enable custom memory manager on systems that allow it (default=disabled)]),,
enable_memmanager=no)

AC_ARG_ENABLE(sound,
    AC_HELP_STRING([--enable-sound],
    [compile in support for sound (default=enabled)]),,
enable_sound=yes)

AC_ARG_ENABLE(particles,
    AC_HELP_STRING([--enable-particles],
    [compile in support for experimental particle system (default=disabled)]),,
enable_particles=no)

if test x$enable_particles = xyes; then
AC_DEFINE(USEPARTICLES,,enables particle system)
fi

AC_ARG_ENABLE(joystick,
	AC_HELP_STRING([--enable-joystick],
		[enable joystick support (default=enabled)]),,
enable_joystick=yes)

AC_ARG_ENABLE(respawn,
	AC_HELP_STRING([--enable-respawn],
		[enable unsupported, hacky, impossible to deactivate at runtime, deathmatch mode. Use at your own risk. (default=disabled)]),,
enable_respawn=no)

AC_ARG_ENABLE(krawall,
	AC_HELP_STRING([--enable-krawall],
		[enable special visuals for the krawall gaming network (default=disabled)]),,
enable_krawall=no)

dnl start alias group
AC_ENABLE(armathentication,
    [echo $enableval; case "${enableval}" in
       yes) enable_armathentication=yes
	   	;;
       no)  enable_armathentication=no
	    ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-armathentication) ;;
     esac],enable_armathentication=yes
     )

AC_ARG_ENABLE(zonesv1,
	AC_HELP_STRING([--enable-zonesv1],
		[enable deprecated zones v1 code (default=disabled)]),,
enable_zonesv1=no)

AC_ARG_ENABLE(zonesv2,
	AC_HELP_STRING([--enable-zonesv2],
		[enable experimental zonesv2 code (default=enabled)]),,
enable_zonesv2=yes)

AC_ARG_ENABLE(fortress,
	AC_HELP_STRING([--enable-fortress],
		[enable fortress support (default=enabled)]),,
enable_fortress=$enable_zonesv2)

AC_ARG_ENABLE(authentication,
    AC_HELP_STRING([--enable-authentication],
		[enable server side code for the krawall style user authentication (default=disabled)]),
    [case "${enableval}" in
       yes) enable_armathentication=yes
	   	;;
       no)  enable_armathentication=no
	    ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-authentication) ;;
     esac],
    )
dnl end alias group

AC_ARG_ENABLE(automakedefaults,
	AC_HELP_STRING([--enable-automakedefaults],
		[enforce the default installation directories as set by automake. localstatedir=prefix/var violates the FHS, so this is off by default.]),,
enable_automakedefaults=no enable_automakedefaults_default=yes)

AC_ARG_ENABLE(dirty,
	AC_HELP_STRING([--enable-dirty],
		[allow dirty GL initialisation (like used with SDL 1.0) even if you have SDL 1.1 or up (default=disabled)]),,
enable_dirty=no)

AC_ARG_ENABLE(sysinstall,
	AC_HELP_STRING([--enable-sysinstall],
		[will install configuration files to /etc/armagetron and create a user if possible (default=enabled)]),,
enable_sysinstall=yes)

AC_ARG_ENABLE(useradd,
	AC_HELP_STRING([--enable-useradd],
		[will create a user during installation (default=enabled; requires --enable-sysinstall or a manual call of the sysinstall script)]),,
enable_useradd=yes enable_useradd_default=yes)
AC_SUBST(enable_useradd)

AC_ARG_ENABLE(etc,
	AC_HELP_STRING([--enable-etc],
		[will create links in /etc to the configuration files (default=enabled; requires --enable-sysinstall or a manual call of the sysinstall script)]),,
enable_etc=yes enable_etc_default=yes)
AC_SUBST(enable_etc)

AC_ARG_ENABLE(desktop,
	AC_HELP_STRING([--enable-desktop],
		[will try to integrate into your desktop environment via menu entries (default=enabled; requires --enable-sysinstall or a manual call of the sysinstall script)]),,
enable_desktop=yes)
AC_SUBST(enable_desktop)

AC_ARG_ENABLE(ruby,
	AC_HELP_STRING([--enable-ruby],
		[enable ruby scripting api.  Requires ruby to be installed. (default=disabled)]),,
enable_ruby=no)
AC_SUBST(enable_ruby)
AM_CONDITIONAL(ENABLE_RUBY, test "x$enable_ruby" = "xyes")

initdir=location
AC_ARG_ENABLE(initscripts,
	AC_HELP_STRING([--enable-initscripts=location],
		[will install init scripts for the servers into <location>(default=enabled; requires --enable-sysinstall; <location> defaults to /etc/init.d or /etc/rc.d)]),
test "x${enableval}" = "xyes" || initdir=${enableval}
test "x${enableval}" = "xno" || enable_initscripts=yes,
enable_initscripts=yes enable_initscripts_default=yes
# disable initscripts if no server is beeing built
test x$build_dedicated = xfalse && test x$armamaster = xfalse && enable_initscripts=no
initdir=location)
AC_SUBST(enable_initscripts)

AC_ARG_ENABLE(restoreold,
	AC_HELP_STRING([--enable-restoreold],
		[will try to install links to previous installation on uninstallation in multiver mode (default=enabled; requires --enable-sysinstall or a manual call of the sysinstall script)]),,
enable_restoreold=yes)
AC_SUBST(enable_restoreold)

AC_ARG_ENABLE(uninstall,
	AC_HELP_STRING([--enable-uninstall@<:@=cmd@:>@],
		[will install an uninstaller script (default=enabled)]),
if test "$enableval" = "no"; then
	enable_uninstall=no
else
	enable_uninstall=yes
	external_uninstall_cmd=$enableval
fi,
enable_uninstall=yes)
AC_SUBST(external_uninstall_cmd)

AC_ARG_ENABLE(multiver,
	AC_HELP_STRING([--enable-multiver@<:@=slot@:>@],
		[allow multiple versions to coexist by storing them in different directories (default=disabled)]),
if test "$enableval" = "no"; then
	enable_multiver=no
else
	enable_multiver=yes
	multiver_slot="$enableval"
fi,
enable_multiver=no)

AC_ARG_ENABLE(games,
	AC_HELP_STRING([--enable-games],
		[decorates all installation paths with "games" (default=enabled)]),,
enable_games=yes)

AC_ARG_ENABLE(enginecoredebug,
     AC_HELP_STRING([--enable-enginecoredebug],
		[Apply debugging options to engine core (for DEBUGLEVEL>0, defaults to enabled)]),
     [case "${enableval}" in
       yes) enginecoredebug=true ;;
       no)  enginecoredebug=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-enginecoredebug) ;;
     esac],[enginecoredebug=true])
	 
AM_CONDITIONAL(ENGINECOREDEBUG, test x$enginecoredebug = xtrue)

if test x$enable_respawn = xyes; then
AC_DEFINE(RESPAWN_HACK,,enables hacky support for deathmatch mode)
fi

if test x$enable_krawall = xyes; then
AC_DEFINE(KRAWALL,,enables krawall)
fi

if test x$enable_armathentication = xyes; then
AC_DEFINE(KRAWALL_SERVER,,enables krawall server)
fi

if test x$enable_zonesv1 = xyes && test x$enable_fortress = xyes; then
	AC_MSG_WARN([
	Zones v2 Fortress is incompatible with Zones v1.
	Therefore, Zones v1 will be disabled.
	])
	enable_zonesv1=no
fi

if test x$enable_zonesv1 = xyes; then
AC_DEFINE(ENABLE_ZONESV1,,enables zonesv1)
fi

AM_CONDITIONAL(BUILDZONESV1, test x$enable_zonesv1 = xyes)

if test x$enable_zonesv2 != xyes && test x$enable_fortress = xyes; then
	AC_MSG_WARN([
	Fortress requires support for zones.
	Enabling zones support.
	])
	enable_zonesv2=yes
fi

if test x$enable_zonesv2 = xyes; then
AC_DEFINE(ENABLE_ZONESV2,,enables zonesv2)
fi

AM_CONDITIONAL(BUILDZONESV2, test x$enable_zonesv2 = xyes)

AM_CONDITIONAL(BUILD_FORTRESS, test x$enable_fortress = xyes)

dnl developer CXXFLAGS
test -z "$DEBUGLEVEL" && DEBUGLEVEL=0
test -z "$CODELEVEL" && CODELEVEL=0

dnl default CXX flags: optimize a bit if no optimization optimization has been selected yet
test -z "`echo $CXXFLAGS | grep "\-O"`" && CXXFLAGS="$CXXFLAGS -O2"

test $DEBUGLEVEL -ge 1 && CXXFLAGS="`echo $CXXFLAGS | sed -e 's,-O.,,'` -g -O0"
test $DEBUGLEVEL -ge 2 && CXXFLAGS="$CXXFLAGS -DDEBUG"
test $DEBUGLEVEL -ge 3 && enable_memmanager=yes
test $DEBUGLEVEL -ge 4 && CXXFLAGS="$CXXFLAGS -DDEBUG_EXPENSIVE"
test $DEBUGLEVEL -ge 5 && CXXFLAGS="$CXXFLAGS -DMEM_DEB"

dnl oddly, this macro needs to be called once unconditionally, or automake
dnl borks up. Backup CXXFLAGS and restore them in case the test passes. Which it does
dnl for some versions of GCC.
CXXFLAGS_BACK="${CXXFLAGS}"
AX_CXXFLAGS_GCC_OPTION(-nobodyreadsconfigureoutputanyway)
CXXFLAGS="${CXXFLAGS_BACK}"

if test $CODELEVEL -ge 1; then
    CXXFLAGS="$CXXFLAGS -Wall"
    
    dnl careful, only supported on GCC 4.2 and later
    AX_CXXFLAGS_GCC_OPTION(-Wno-strict-overflow)
fi

dnl test $CODELEVEL -ge 2 && CXXFLAGS="$CXXFLAGS -Wshadow -Wpointer-arith -Wfloat-equal -W -Wcast-qual -Wcast-align -Wconversion -Wunreachable-code --pedantic -Winline"
test $CODELEVEL -ge 2 && CXXFLAGS="$CXXFLAGS -Wpointer-arith -W -Wcast-qual -Wcast-align --pedantic -Wno-unused-parameter"
test $CODELEVEL -ge 3 && CXXFLAGS="$CXXFLAGS -Weffc++ -Wshadow -Winline -Wfloat-equal -Wconversion"
test $CODELEVEL -ge 4 && CXXFLAGS="$CXXFLAGS -Werror"

dnl determine whether "games" should be put into paths: not if it was
dnl requested with --disable-games or if "games" is already included
dnl in the data path
games_suffix_data=/
games_suffix_doc=/
games_suffix_config=/
games_suffix_var=/
if test "${enable_games}" = "yes"; then
      test -z "`echo ${datadir}       | grep games`" && games_suffix_data=/games/
      test -z "`echo ${prefix}        | grep games`" && games_suffix_doc=/games/
      test -z "`echo ${sysconfdir}    | grep games`" && games_suffix_config=/games/
      test -z "`echo ${localstatedir} | grep games`" && games_suffix_var=/games/
fi
AC_SUBST(games_suffix_data)
AC_SUBST(games_suffix_config)
AC_SUBST(games_suffix_doc)
AC_SUBST(games_suffix_var)

dnl determine memmanager state
if test x$enable_memmanager != xyes ; then
  AC_DEFINE(DONTUSEMEMMANAGER)
fi

dnl LibTool configuration
dnl AC_LIBTOOL_DLOPEN
dnl AM_PROG_LIBTOOL

dnl Header file
AC_CONFIG_HEADER(aa_config.h)


dnl Checks for programs.

dnl C Compiler:
AC_PROG_CC
AC_PROG_CPP

dnl C++ Compiler:
AC_PROG_CXX
AC_PROG_CXXCPP
dnl AC_OBJEXT

dnl Make:
dnl AC_PROG_MAKE_SET

AC_CHECK_PROG(HAVE_GMAKE, gmake, yes, no)
if test ${HAVE_GMAKE} = no && make --version | head -n 1 | grep -v "GNU make" 2>&1 /dev/null; then
   AC_MSG_ERROR([GNU make (gmake) is currently required for compilation.])
fi

AC_CHECK_PROG(HAVE_BASH, bash, yes, no)
if test ${HAVE_BASH} = no; then
   AC_MSG_ERROR([GNU Bourne-Again SHell (bash) is currently required for compilation.])
fi

dnl Boost
AX_BOOST_BASE([1.33.1])

dnl Ruby

AC_ARG_WITH(ruby,
    AC_HELP_STRING([--with-ruby],
        [The ruby to use]),
    if test ! -z "${withval}"; then
        RUBY="${withval}"
    fi
)

dnl glew

have_glew=true

AC_ARG_WITH(glew,
    AC_HELP_STRING([--with-glew],
        [Use GLEW and the OpenGL extensions it provides]),
        test "${withval}" == "yes" || have_glew=false
)

dnl

dnl testing a nifty make feature: silent inclu[]des
AC_SINCLUDE

dnl determine the program name and title
if test "x$progtitle" = "x" ; then
  progtitle="Armagetron Advanced"
fi

if test "x$progname" = "x" ; then
  progname=armagetronad
fi

prognamebase=$progname
AC_SUBST(progtitle)
AC_SUBST(progname)
AC_SUBST(prognamebase)

AM_CONDITIONAL(USE_SYSINSTALL, test "$enable_sysinstall" = "yes")
AM_CONDITIONAL(USE_UNINSTALL, test "$enable_uninstall" = "yes")

AM_CONDITIONAL(ALLOW_MULTIPLE_VERSIONS, test "$enable_multiver" = "yes")
AC_SUBST(enable_multiver)
if test "$enable_multiver" = "yes"; then
  if test "$multiver_slot" = "yes"; then
    multiver_slot="$PACKAGE_VERSION"
  fi
   progdir_suffix="-$multiver_slot"
   executable_suffix="-$multiver_slot"
else
   progdir_suffix=""
   executable_suffix=""
fi
AC_SUBST(progdir_suffix)
AC_SUBST(executable_suffix)

dnl AC_DEFINE(PROGDIR_SUFFIX,\"$progdir_suffix\",[Define to the suffix of all installation folders])
AC_DEFINE_UNQUOTED(PROGDIR_SUFFIX,"${progdir_suffix}",[Define to the suffix of all installation folders])

dnl type checks
AC_CHECK_TYPE(socklen_t,AC_DEFINE(HAVE_SOCKLEN_T, 1, [Define if you have the type socklen_t.]),,[#include <sys/types.h>
#include <sys/socket.h>])

# tells the makefile if the data files need to be installed
needdata=yes
AC_SUBST(needdata)

test "$docstyle" = "" && docstyle=unix

# find good default of init script directory
initdirdefault=/etc/init.d
for candidate in /etc/rc.d /etc/init.d; do
    test -d ${candidate} && initdirdefault=${candidate}
done

# configuration for various unix flavours
define(GENERICUNIX,[
        MATHLIB="m"
        if test x$build_regular = xtrue; then
                AC_PATH_X
                AC_PATH_XTRA
        fi
        if test x$have_x = xyes; then
            CFLAGS="$CFLAGS $X_CFLAGS"
            CXXFLAGS="$CXXFLAGS $X_CFLAGS"
            LIBS="$X_LIBS $LIBS"
            SYS_GL_LIB="GL"
            SYS_GLU_LIB="GLU"
        else
            SYS_GL_LIB="GL"
            SYS_GLU_LIB="GLU"
        fi
]
)

define(SAFEUNIX,[
        AC_DEFINE(DONTUSEMEMMANAGER, 1, [Define if you dont want to use a custom memory manager.])
GENERICUNIX
]
)

SDL_CONFIG_DEFAULT=sdl-config

dnl *************************************************
dnl GL AND  MATHLIB SELECTION ( SYSTEM DEPENDANT)
dnl *************************************************
case "$target" in
    *-*-cygwin* | *-*-mingw32*)
	AC_DEFINE(WIN32, 1, [Windows 9x/NT/2k/XP])
        MATHLIB=""
        SYS_GL_LIB="opengl32"
        SYS_GLU_LIB="glu32"
        LIBS="$LIBS -lwsock32"
        CFLAGS="$CFLAGS -DNO_SOCKLEN_T"
        ;;
    *-*-beos*)
	AC_DEFINE(BEOS, 1, [BeOS])
        MATHLIB=""
        SYS_GL_LIB="GL"
        SYS_GLU_LIB="GLU"
        ;;
    *-*-darwin*)
	AC_DEFINE(MACOSX, 1, [Mac OS X])
        MATHLIB=""
        SYS_GL_LIB=""
        SYS_GLU_LIB=""
        #LIBS="$LIBS -framework GLU -framework Quicktime -lsmpeg"
        ;;
    *-*-aix*)
	AC_DEFINE(AIX, 1, [AIX])
        MATHLIB=""
        if test x$ac_cv_prog_gcc = xyes; then
            CXXFLAGS="$CXXFLAGS -mthreads"
	fi
        SYS_GL_LIB=""
        SYS_GLU_LIB=""
        ;;
    *-*-solaris*)
	AC_DEFINE(SOLARIS, 1, [Solaris])
	AC_DEFINE(CAUTION_WALL, 1, [Define if your system deos not like the pointer tricks in eWall.h.])
        LIBS="$LIBS -lnsl -lsocket"
SAFEUNIX
        ;;
    *-*-linux* )
    	AC_DEFINE(LINUX, 1, [GNU/Linux])
GENERICUNIX
		;;
    *-*-openbsd* )
        # OpenBSD, almost like generic Unix, but needs different init script handling
        initdirdefault=OpenBSD
SAFEUNIX
        ;;
    *)
        # generic unix
SAFEUNIX
        ;;
esac

test -z "$SDL_CONFIG" && SDL_CONFIG=$SDL_CONFIG_DEFAULT

AC_SUBST(docstyle)

#processor specific setup
case "$target" in
    i*86-*-*)
        ;;
   	x86_64-*-*)
        ;;
    sparc-*-*)
        ;;
    *)
		#disable memory manager; it may depend on processor features
		AC_DEFINE(DONTUSEMEMMANAGER)
        ;;
esac

AC_CACHE_SAVE

dnl *************************************************
dnl         libxml2
dnl *************************************************

if test x$armamain = xtrue; then

test -z "$XML2_CONFIG" && XML2_CONFIG=xml2-config

if ! $XML2_CONFIG --libs > /dev/null; then
  AC_MSG_ERROR([Fatal error: the libxml2 configuration script $XML2_CONFIG does not work/exist.])
fi

# if test x$armamain = xtrue; then
AC_CHECK_LIB(xml2,xmlParseMemory,
CPPFLAGS="$CPPFLAGS `$XML2_CONFIG --cflags`"
LIBS="`$XML2_CONFIG --libs` $LIBS"
AC_DEFINE(HAVE_LIBXML2, 1, [Define if you have the xml2 library (-lxml2).]),
AC_MSG_ERROR([You need the library xml2 to compile Armagetron. Read the file doc/install_linux.html.])
,`$XML2_CONFIG --libs`)

AC_CHECK_FUNC(xmlParserInputBufferCreateFilenameDefault,
,
AC_MSG_ERROR([
	Your xml2 library is obsolete!
	Please upgrade to 2.6.11 or later
])
)

# fi

dnl *************************************************
dnl         Armathentication Threading
dnl *************************************************

if test x$enable_armathentication = xyes; then

AC_LANG([C++])

ACX_PTHREAD([

LIBS="${PTHREAD_LIBS} ${LIBS}"
CXXFLAGS="${CXXFLAGS} ${PTHREAD_CFLAGS}"
CC="${PTHREAD_CC}"
dnl AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.])

],[

AM_PATH_ZTHREAD_AA(2.3.2)
CXXFLAGS="${CXXFLAGS} ${ZTHREAD_CXXFLAGS}"
LIBS="${LIBS} ${ZTHREAD_LIBS}"

])

fi # krawall


dnl *************************************************
dnl         pattern buffers
dnl *************************************************

dnl check for compiler
AC_CHECK_PROG([PROTOC],protoc,protoc,no)

AC_LANG([C])

dnl check for library
AC_CHECK_LIB([protobuf],_ZN6google8protobuf7MessageD2Ev,,
[
	AC_CHECK_LIB([protobuf],_ZN6google8protobuf7ClosureD2Ev,
		AC_MSG_WARN([[The library protobuf (Google Protobuf Buffers) has a broken dependency on library pthread on your system.]])
		AC_DEFINE_UNQUOTED(HAVE_LIBPROTOBUF)
		LIBS="-lprotobuf -lpthread $LIBS"
	,
AC_MSG_ERROR([[The library protobuf (Google Protocol Buffers, http://code.google.com/p/protobuf/) is required, but was not found.]])
	,[-lpthread]
	)
]
)

AC_LANG([C++])

dnl check for header
AC_CHECK_HEADERS([google/protobuf/message.h],,
AC_MSG_ERROR([[The library protobuf (Google Protocol Buffers, http://code.google.com/p/protobuf/) is required with all development files, but was not found.]])
)

dnl AC_LANG([C])

fi # armamain

dnl *************************************************
dnl         math
dnl *************************************************

if test $MATHLIB ; then
AC_CHECK_LIB($MATHLIB, exp,,
	AC_MSG_WARN([
	You do not have the standard math library libm. Check
	config.log for error messages and fix that.]
	)
)
fi

if test x$build_regular = xfalse; then
 AC_MSG_RESULT([

 Configuring dedicated server, skipping libraries...
 ])

dnl ****************************************************
dnl Put stuff needed for the dedicated server here
dnl ****************************************************

 AC_DEFINE(DEDICATED,,[Define if you wish to compile a dedicated server])

 AC_DEFINE(NOJOYSTICK,,[No joysticks in the dedicated server])
 enable_sound=no
 progname=${prognamebase}-dedicated
 AC_SUBST(progname)

 needdata=no
 AC_SUBST(needdata)
else


if test x$enable_dirty = xyes; then
  AC_DEFINE(DIRTY,,[Define if you wish to use the old and dirty OpenGL
  initialization method])
fi


dnl *************************************************
dnl         SDL
dnl *************************************************

test -z "$SDL_CONFIG" && SDL_CONFIG=sdl-config

AC_MSG_CHECKING(whether SDL is installed)
if SDLLIBS=`${SDL_CONFIG} --libs`; then
    AC_MSG_RESULT(yes)
fi

CPPFLAGS="$CPPFLAGS `${SDL_CONFIG} --cflags`"
LIBS="${SDLLIBS} $LIBS"

AC_CHECK_FUNC(SDL_Init,
AC_DEFINE(HAVE_LIBSDL,,[Define if you have the library SDL]),
AC_MSG_ERROR([You need the library SDL to compile Armagetron. Read the file doc/install_linux.html.])
,)

dnl ***************** OPTIONAL STUFF AFTER THIS POINT **********************

dnl *************************************************
dnl         Sound
dnl *************************************************

if test x$enable_sound = xyes; then

dnl first do the regular sdl define
 AC_DEFINE(HAVE_SDL_FOR_SOUND, 1,  [Define if you have SDL_sound, which is part of the core SDL])

dnl now check for sdl_mixer
 AC_CHECK_LIB(SDL_mixer,Mix_OpenAudio,
 AC_DEFINE(HAVE_LIBSDL_MIXER, 1, [Define if you have the SDL_mixer library (-lSDL_mixer).])
 LIBS="-lSDL_mixer $LIBS",
 AC_MSG_ERROR([SDL_mixer library not found. It is required for now. Sorry.])
 ,)

dnl now we need to know which audio formats to use

fi

dnl *************************************************
dnl             Joystick support
dnl *************************************************

if test x$enable_joystick = xno; then
	AC_DEFINE(NOJOYSTICK, 1, [Define if you don't want joystick support.])
fi

dnl *************************************************
dnl         GL
dnl *************************************************

if test $SYS_GL_LIB && echo $LIBS | grep -v "\-lGL" > /dev/null; then

dnl AC_CHECK_LIB($SYS_GL_LIB, glEnd,,

AC_CHECK_LIB($SYS_GL_LIB, glVertex3f,,
 AC_MSG_RESULT([OpenGL not found. Maybe it needs X11 to compile? Checking that...])
 AC_CHECK_LIB(X11,main,
  LIBS="-lX11 -L/usr/X11R6/lib $LIBS"
  AC_DEFINE(HAVE_LIBX11, 1, [Define if you have the X11 library (-lX11).])
  ,
  AC_MSG_WARN([
  Standard X11 library needed by OpenGL not found.
  ]),-L/usr/X11R6/lib)
 AC_CHECK_LIB(Xext,main,-L/usr/X11R6/lib)
  AC_CHECK_LIB(Xt,main,-L/usr/X11R6/lib)
 AC_CHECK_LIB(Xi,main,-L/usr/X11R6/lib)
 AC_CHECK_LIB(Xmu,main,-L/usr/X11R6/lib)
AC_CHECK_LIB($SYS_GL_LIB, glBegin,,AC_MSG_ERROR([
  You need Mesa or an OpenGL-System to compile Armagetron.
  Maybe your libGL needs the X libraries and your system does not
  find them? Read the file doc/install_linux.html.]),-L/usr/X11R6/lib)
)

dnl check for 3DFX-MESA
dnl AC_CHECK_LIB(GL,XMesaSetFXmode,AC_DEFINE(HAVE_FXMESA),,)

fi

dnl *************************************************
dnl         GLU
dnl *************************************************


if test $SYS_GLU_LIB && echo $LIBS | grep -v "\-lGLU" > /dev/null; then

dnl AC_CHECK_LIB($SYS_GLU_LIB, gluBuild2DMipmaps,,
AC_CHECK_LIB($SYS_GLU_LIB, gluBuild3DMipmaps,,
AC_MSG_ERROR([
You need GLU to compile Armagetron; it SHOULD have come with OpenGL.
Read the file doc/install_linux.html.]))

dnl *************************************************
dnl         GLEW
dnl *************************************************

if test ${have_glew} = true; then
  AC_CHECK_LIB(GLEW,glewInit,,
  AC_MSG_WARN([OpenGL Extension Wrangler (GLEW) library not found. No fancy effects for you.])
  have_glew=false
  )
fi

if test ${have_glew} = true; then
  AC_CHECK_HEADERS([GL/glew.h],,
  AC_MSG_WARN([OpenGL Extension Wrangler (GLEW) headers not found. No fancy effects for you.])
  have_glew=false
  )
fi

dnl *************************************************
dnl         FTGL
dnl *************************************************

AC_LANG([C++])

test -z "$FREETYPE_CONFIG" && FREETYPE_CONFIG=freetype-config

FTGL_CFLAGS="`pkg-config ftgl --cflags`"
FTGL_LIBS="`pkg-config ftgl --libs`"

FTGL_ERROR="You need the library ftgl in at least version 2.1.3~rc5 to compile Armagetron. Read the file doc/install_linux.html."

AC_CHECK_LIB(ftgl,ftglRenderGlyph,
CPPFLAGS="$CPPFLAGS $FTGL_CFLAGS"
LIBS="$LIBS $FTGL_LIBS"
,
AC_MSG_ERROR([$FTGL_ERROR]),
$FTGL_LIBS
)

USE_FTGL=yes

# `pkg-config ftgl --cflags` doesn't reliably give the include path to FTGL itself, we are on our own finding the includes in some cases.
# look for FTGL headers in default system header locations and the prefixes of the libraries that do have a -config script
FTGL_HEADERS="/usr/include/FTGL /usr/local/include/FTGL `$FREETYPE_CONFIG --prefix`/include/FTGL `$SDL_CONFIG --prefix`/include/FTGL `$XML2_CONFIG --prefix/`include/FTGL ${prefix}/include/FTGL "

AC_ARG_WITH(ftgl,
    AC_HELP_STRING([--with-ftgl],
        [use FTGL for rendering (default=enabled, which is currently required), the argument is the location of the FTGL headers]),
    if test "${withval}" = yes -o "${withval}" = no; then
        USE_FTGL="${withval}"
    else
        FTGL_HEADERS=${withval}
    fi
)

# candidates are collected, walk through them
if test -z "`echo $FTGL_CFLAGS | grep FTGL`"; then
    found=false
    AC_MSG_CHECKING(for location of FTGL includes)
    for candidate in ${FTGL_HEADERS}; do
        if test -r ${candidate}/FTGL.h || test -r ${candidate}/ftgl.h; then
            AC_MSG_RESULT(${candidate})
                CPPFLAGS="$CPPFLAGS -I${candidate}"
                found=true
                break
            fi
    done

    if test ${found} = false; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([FTGL headers not found on your system. Please pass the correct location via --with-ftgl=<path to FTGL include directory>.])
    fi
fi

dnl *************************************************
dnl         freetype
dnl *************************************************

dnl FTGL has this one in its dependencies and libraries it passes via pkg-config, but
dnl they're in the wrong order for static linking. That's why we add freetype once again.

AC_CHECK_LIB(freetype,FT_Init_FreeType,
CPPFLAGS="$CPPFLAGS `$FREETYPE_CONFIG --cflags`"
LIBS="$LIBS `$FREETYPE_CONFIG --libs`"
,
AC_MSG_ERROR([You need the library freetype to compile Armagetron. Read the file doc/install_linux.html.])
)

dnl FTGL again. Check for the headers. First, try the individual header.
dnl Use modified pedantic CXXFLAGS for that. If the headers are deprecated,
dnl the test will fail.
CXXFLAGS_SAVE="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Werror -pedantic"
AC_MSG_CHECKING([whether FTGLPixmapFont.h is deprecated])
AC_TRY_COMPILE([#include <FTGLPixmapFont.h>],return 0,[AC_MSG_RESULT([no, not checking <ftgl.h>.])],
[AC_MSG_RESULT([yes])
AC_CHECK_HEADERS(ftgl.h)]
)
CXXFLAGS="$CXXFLAGS_SAVE"

# end of dedicated server if/else
fi

dnl *************************************************
dnl         Other Stuff
dnl *************************************************

dnl AC_CHECK_LIB(g++, main,,
dnl AC_MSG_WARN([C++ standard library not found. You can ignore this warning on non-GNU-systems.]))

AC_LANG([C])

AC_CHECK_LIB(z, main,,
AC_MSG_ERROR([You need libz to compile Armagetron.]))

AC_MSG_CHECKING(whether libpng is installed)
if PNGLIBS=`libpng-config --libs`; then
    AC_MSG_RESULT(yes)
fi

CPPFLAGS="$CPPFLAGS `libpng-config --cflags`"
LIBS="$LIBS $PNGLIBS"

AC_CHECK_FUNC(png_check_sig,
,
AC_MSG_ERROR([You need libpng to compile Armagetron.]))

AC_CHECK_LIB(jpeg, main,,
	AC_MSG_WARN([
	libjpeg not found. Expect small graphical errors ( missing moviepack title screen ).]
	)
)

AC_CHECK_LIB(SDL_image, IMG_Load,,
dnl AC_CHECK_LIB(IMG, main,,
AC_MSG_ERROR([You need the library SDL_image to compile Armagetron.]
dnl )
))

dnl look for the right header for SDL_image
AC_CHECK_HEADERS(SDL_image.h,,
[
AC_MSG_WARN([SDL_image header not found where it should be ( the SDL include directory ).
Trying fallback.])
AC_CHECK_HEADERS(SDL/SDL_image.h,,
[AC_MSG_WARN([
SDL_image header not found. The library is there, so something must be unusual
in your setup: SDL_image.h does not reside in the same folder as the rest of the SDL
inlcudes and not in any SDL subdirectory in the inclue path. If you know the
correct path to it, edit src/render/rTexture.cpp and enter it there.])]
)
]
)

ifelse(,,,
[
dnl look for the right way to include the opengl headers
AC_CHECK_HEADERS( SDL_opengl.h,,
AC_CHECK_HEADERS( SDL/SDL_opengl.h,,
AC_CHECK_HEADERS( GL/gl.h,,
AC_CHECK_HEADERS( OpenGL/gl.h,,
AC_MSG_ERROR([OpenGL header not found.])
)
)
)
)
])

export CPPFLAGS
export CXXFLAGS

fi dnl (dedicated server)



dnl Replace `main' with a function in -lpthread:

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Checks for library functions.
AC_CHECK_FUNCS(select)
AC_CHECK_FUNCS(atan2f sinf cosf tanf sqrtf logf expf fabsf floorf pow10 pow10f isblank)
AC_CHECK_FUNCS(wmemset)

# if the compiler is GCC -Os produces faulty code (exceptions cause crashes), at least on AMD64. Replace it with -O2.
test -z "`$CXX --version | grep '(GCC)'`" || CXXFLAGS="`echo $CXXFLAGS | sed -e 's,-Os,-O2,g' `" CFLAGS="`echo $CFLAGS | sed -e 's,-Os,-O2,g' `"

dnl add current directories to include search path
CPPFLAGS="-I. -I.. $CPPFLAGS"

if test "$enable_sound" = "yes"; then
AC_DEFINE(HAVE_LIBSDL_MIXER, 1, [Use SDL_mixer])
fi

# define
AC_DEFINE_UNQUOTED(TOP_SOURCE_DIR,"${srcdir}", [Define the top source directory])
AC_DEFINE_UNQUOTED(PROGNAME,"${progname}", [Define the short progam name (with possible -dedicated suffix)])
AC_DEFINE_UNQUOTED(PROGNAMEBASE,"${prognamebase}", [Define the short progam name])

realprefix=${prefix}
test x${realprefix} = xNONE && realprefix=${ac_default_prefix}
AC_DEFINE_UNQUOTED(PREFIX,"${realprefix}",[Define to the compiled in prefix where most game directories will be children of])
realexecprefix=${exec_prefix}
test x${realexecprefix} = xNONE && realexecprefix=${realprefix}

AM_CONDITIONAL(NEEDDATA, test "$needdata" = "yes" )
AM_CONDITIONAL(USE_SDLMIXER, test "$enable_sound" = "yes" )

#test universal variables
dnl univar1=\${prefix}/foo
dnl univar2=bar
dnl AC_SUBST_UNIVERSAL(univar1)
dnl AC_SUBST_UNIVERSAL(univar2)
dnl AC_AA_PATH(tridir,${prefix}/tri,[trinary data],[PREFIX/tri])

# set and export init directory
test "x$initdir" = "xlocation" && initdir=${initdirdefault}
AC_SUBST(initdir)
AC_AA_PATH_NOSUFFIX_RAW(initdir)

if test "x$enable_automakedefaults" = "xno"; then
    # DEVIATION FROM AUTOMAKE DEFAULT for better FHS compatibility:
    # automake puts localstatedir to ${prefix}/var. This violates
    # the FHS condition that /usr needs to be mountable read-only
    # (except for installing/uninstalling software), so we put it
    # by default to /var. Relocation does not suffer as no files
    # are installed there initially.
    if test "x$build_dedicated" = "xtrue" || test "x$armamaster" = "xtrue"; then
        AC_MSG_CHECKING(whether to bend localstatedir to /var)
        if test "x${localstatedir}" = "x\${prefix}/var"; then
            if test -d `eval echo ${localstatedir}`; then
                AC_MSG_RESULT([no, the automake default exists])
            else
                AC_MSG_RESULT(yes)
                localstatedir=/var
                test "x$enable_automakedefaults_default" = "xyes" && OUTSIDE_OF_PREFIX=yes
            fi
        else
            AC_MSG_RESULT([no, it has been overridden by the user])
        fi
    fi
    # this is not the best way to do it as the user has to set
    # --localstatedir=/usr/local/var and not --localstatedir=\${prefix}/var
    # to override it, but I could not find anything better.

    # ANOTHER DEVIATION: if --enable-games is set and the bindir
    # is the default, try to find an alternative binary directory
    # that also lies in the path, but has /games in it.
    if test "x${bindir}" = "x\${exec_prefix}/bin" && test "x$enable_games" = "xyes"; then
        for alt in bin/games games games/bin; do
            AC_MSG_CHECKING([alternative binary path ${realexecprefix}/${alt}])
            if test -d ${realexecprefix}/${alt} && echo ${PATH} | grep ${realexecprefix}/${alt} > /dev/null; then
                bindir=\${exec_prefix}/${alt}
                AC_MSG_RESULT(possible)
            else
                AC_MSG_RESULT(no)
            fi
        done
        # echo binary path : ${bindir}
    fi
fi

# paths given by autoconf relevant to us
# raw paths. No suffix, nothing to do but make them known
AC_AA_PATH_NOSUFFIX_RAW(prefix)
AC_AA_PATH_NOSUFFIX_RAW(exec_prefix)

# check whether those are part of suffix and disable relocability if not
AC_AA_PATH_PREPARE_BINRELOC(bindir,\${exec_prefix})
AC_AA_PATH_NOSUFFIX_RAW(bindir)
AC_AA_PATH_PREPARE_BINRELOC(sbindir,\${exec_prefix})
AC_AA_PATH_NOSUFFIX_RAW(sbindir)

# the full program on those: add (/games)/armagetronad(-dedicated) suffix,
# check relocability. The suffix can be overridden any time, before or after
# this block.
AC_AA_PATH_PREPARE_BINRELOC(datadir,\${prefix})
AC_AA_PATH_RAW(datadir)
AC_AA_PATH_PREPARE_BINRELOC(sysconfdir,\${prefix})
AC_AA_PATH_RAW(sysconfdir)
AC_AA_PATH_PREPARE(localstatedir)
AC_AA_PATH_RAW(localstatedir)
AC_AA_PATH_PREPARE(infodir)
AC_AA_PATH_RAW(infodir)
AC_AA_PATH_PREPARE(mandir)
AC_AA_PATH_RAW(mandir)

# additional paths:
# scripts
AC_AA_PATH_NOSUFFIX(scriptdir,${aa_datadir}/scripts,[shell scripts],[DATADIR/scripts])
AC_AA_PATH_NOSUFFIX(wwwrootdir,${aa_datadir}/www-root,[web root],[DATADIR/www-root])

# documentation
AC_AA_PATH_PREPARE(docdir)
AC_AA_PATH(docdir,${datadir}/doc,[documentation],[DATADIR/doc])

# PID files
AC_AA_PATH_PREPARE(rundir)
rundir_suffix=""
AC_AA_PATH(rundir,${localstatedir}/run,[PID files],[LOCALSTATEDIR/run])

# log files
AC_AA_PATH_PREPARE(logdir)
logdir_suffix=""
AC_AA_PATH(logdir,${localstatedir}/log,[server logs],[LOCALSTATEDIR/log])

# state files of previous version to move to new location
oldvardirdefault=/var/${progname}/var
test "x$enable_games" = "xyes" && oldvardirdefault=/var/games/${progname}/var
AC_AA_PATH_WITH(oldvardir,\${prefix}${oldvardirdefault},[directory where the state files of a previous version may be kept; they'll be moved to their new place on installation.],[oldvardir])
AC_ARG_ENABLE(migratestate,AC_HELP_STRING([--enable-migratestate],
        [tries to move old server state data to its new location (default=enabled)]),,
        enable_migratestate=yes)
AC_SUBST_UNIVERSAL(enable_migratestate)

# determine whether binreloc should be used
# this needs to come after all AC_AA_PATH_PREPARE_BINRELOC macros.
AM_BINRELOC

# disable user addition for client
test x$build_dedicated = xtrue -o x$armamaster = xtrue || enable_useradd=no enable_useradd_default=no

# export relocatability to all systems (doesn't strictly have to
# go through the 'determine at build time' mechanism, but it doesn't hurt.
relocatable=${br_cv_binreloc}
AC_SUBST_UNIVERSAL(relocatable)

AC_USE_SUBST_UNIVERSAL
AC_AA_REPLACEPATHS

dnl Strip pedantic flags from general CXXFLAGS and put them into a separate variable
CXXFLAGS_OURFILES=''

if echo ${CXXFLAGS} | grep -- '-Werror' > /dev/null; then
  CXXFLAGS_OURFILES="${CXXFLAGS_OURFILES} -Werror"
fi

if echo ${CXXFLAGS} | grep -- '--pedantic' > /dev/null; then
  CXXFLAGS_OURFILES="${CXXFLAGS_OURFILES} --pedantic"
fi

CXXFLAGS="`echo ${CXXFLAGS} | sed -e 's,-Werror,,g' | sed -e 's,--pedantic,,g'`"

dnl CXX flags for our own hand-written files. Can be pedantic.
AC_SUBST(CXXFLAGS_OURFILES)

export CPPFLAGS
export CXXFLAGS
export LIBS

AC_OUTPUT(src/Makefile
src/protobuf/Makefile
src/first/Makefile
Makefile
batch/Makefile
language/languages.txt
config/rc.config config/aiplayers.cfg
src/doc/Makefile src/doc/net/Makefile src/doc/Doxyfile
src/doc/html.m4
src/thirdparty/Makefile
src/thirdparty/particles/Makefile
src/thirdparty/mathexpr/Makefile
src/thirdparty/scrap/Makefile
src/swig/Makefile
resource/Makefile
config/Makefile
desktop/Makefile
www-root/Makefile
desktop/armagetronad.desktop
desktop/armagetronad.protocol
)

# m4_define(FULLEVAL,stage1=`eval echo $1`; stage2

echo "Configuration complete!"
echo ""
echo "Options selected are:"
echo "     Build Regular Game Client: $build_regular"
echo "     Build Dedicated Server: $build_dedicated"
echo "     Build Master Server: $armamaster"
echo ""
echo "Installation options:"
echo "     Prefix: $prefix"
echo ""
if test x$build_dedicated = xtrue -o x$armamaster = xtrue ;
then
echo "     Build with Armathentication support (server): $enable_armathentication"
echo "     Init scripts will be installed in  : $initdir"
echo "     Dynamic data will be kept in       : `eval echo ${aa_localstatedir}`"
echo "     PID files will be kept in          : `eval echo ${rundir}`"
echo "     Log files will be kept in          : `eval echo ${logdir}`"
echo "     Add user to run server             : $enable_useradd"
echo ""
fi
echo "     Sound support: $enable_sound"
echo "     Particle System support: $enable_particles"
echo "     Joystick support: $enable_joystick"
zones_info="NO"
if test x$enable_zonesv2 = xyes; then
	zones_info="Basic"
	if test x$enable_fortress = xyes; then
		zones_info="${zones_info}, Fortress"
	fi
	if test x$enable_zonesv1 = xyes; then
		zones_info="V1, $zones_info"
	fi
elif test x$enable_zonesv1 = xyes; then
	zones_info="V1"
fi
echo "     Zones support: $zones_info"
echo ""
echo "     Binaries will be installed in      : `eval echo ${bindir}`"
echo "     Static data will be installed in   : `eval echo ${aa_datadir}`"
echo "     Configuration will be installed in : `eval echo ${aa_sysconfdir}`"
echo "     Documentation will be installed in : `eval echo ${aa_docdir}`"
test "x$enable_etc" = "xyes" && echo "     Link to configuration will go to   : /etc/${progname}"
echo "     Build with Krawall authentication support: $enable_krawall"
echo "     Ruby support: $enable_ruby"
if test "x$enable_ruby" = "xyes"; then
echo "         Ruby include dir: $ruby_include_dir"
echo "         Ruby lib dir: $ruby_lib_dir"
fi
echo "     Use dirty SDL initialization: $enable_dirty"
echo "     Call sysinstall: $enable_sysinstall"
if test "$enable_uninstall" = "yes" &&
   test "$external_uninstall_cmd" != ""; then
	uninstall_info=" (external: \"$external_uninstall_cmd\")"
else
	uninstall_info=""
fi
echo "     Generate uninstall: $enable_uninstall$uninstall_info"
if test "$enable_multiver" = "yes"; then
	multiver_info=" (slot \"$multiver_slot\")"
else
	multiver_info=""
fi
echo "     Allow multiple versions: $enable_multiver$multiver_info"
echo "     Use custom memory manager: $enable_memmanager"
echo "     Debug level : $DEBUGLEVEL"
echo "     Code level  : $CODELEVEL"
echo "     CPPFLAGS    : $CPPFLAGS"
echo "     CXXFLAGS    : $CXXFLAGS"
echo "     LIBS        : $LIBS"

# print warning about behavior deviating from automake's defaults
SWITCHES=""
test "x$OUTSIDE_OF_PREFIX" = "xyes" && SWITCHES=" --enable-automakedefaults"
test "x$enable_useradd_default" = "xyes" && OUTSIDE_OF_PREFIX=yes SWITCHES="${SWITCHES} --disable-useradd"
test "x$enable_etc_default" = "xyes" && OUTSIDE_OF_PREFIX=yes SWITCHES="${SWITCHES} --disable-etc"
test x$build_dedicated = xtrue -o x$armamaster = xtrue && test "x$enable_initscripts_default" = "xyes" && echo "${initdir}" | grep '^/etc/' > /dev/null && OUTSIDE_OF_PREFIX=yes SWITCHES="${SWITCHES} --disable-initscripts"
if test "x$OUTSIDE_OF_PREFIX" = "xyes"; then
    echo ""
    echo "Warning: The configure script decided that, to better work on your system,"
    echo "  it would be a good idea to modify files outside of ${prefix}"
    echo "   during installation or while running. If you disagree, rerun configure with"
    echo "   ${SWITCHES}"
    echo "  after consulting \"configure --help\" for the effects; if you want to get rid"
    echo "  of this warning, pass the corresponding opposite switches."
    echo ""
fi

# print warnings about missing optional libraries that cause feature reduction
if test ${have_glew} = false; then
    echo "Warning: GLEW library (http://glew.sourceforge.net/) not found or not functional. Advanced graphics effects will be disabled. Ok, we don't have that many of them anyway, so this waring is safe to ignore for now."
fi
