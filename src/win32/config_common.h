// config include file common for all Windows builds

// map BSD/Linux/OSX string compare function to Win pendant
#define strcasecmp strcmpi
//Required for MSVC? Breaks MingW
//#define vsnprintf _vsnprintf

// for visual studio 2005: use secure template overloads of strcopy and the like
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

// this one is included in winlibs as static library
#define HAVE_LIBBOOST_THREAD
#define BOOST_THREAD_USE_LIB

// and disable warnings about those calls that can't be converted. We may want to look at
// them later, though.
#define _CRT_SECURE_NO_DEPRECATE 1

// activate zones v2 support
#define ENABLE_ZONESV2 1

// required define for addrinfo based functions used in nSocket.cpp, requires Windows 2000
// needs to be here so it is consistent across the whole project and comes before windows.h
#define WINVER 0x0501

// include of windows.h needed for consistency of silly windows #defines ( SetPort -> SetPortA, GetUserName -> GetUserNameA )
#include  <windows.h>

// include common non-autoconf config file
#include "config_ide.h"

// disable long identifier warnings, they are common in STL
#pragma warning ( disable: 4786 )

// disable POD initialization behavior change warning in VisualC++ 2005
#pragma warning ( disable: 4345 )

// compatibility with later mingw versions
#if defined(__GNUC__) && __GNUC__ >= 4
#define HAVE_ISBLANK
#endif

// Define if this is a Windows OS.
#ifndef WIN32
#define WIN32
#endif

// uncomment this line to compile a version that TRIES to be compatible
// with Windows 9X. No guarantees.
// #define SUPPORT_WIN9X

// defines for data directories in Windows
#ifndef DEBUG
#define USER_DATA_DIR  "${APPDATA}/Armagetron"
#define SCREENSHOT_DIR "${MYPICTURES}/Armagetron"
#else
#define USER_DATA_DIR  "."
#endif

// for now, no joystick support in Windows
// #define NOJOYSTICK 1

// Define this for the particle library
#define PARTICLEDLL_EXPORTS

// define version
// #include "../version.h"
