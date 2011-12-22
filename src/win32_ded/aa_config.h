// Windows dedicated server configuration header

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

// define if you wish to compile a dedicated server
#define DEDICATED 1

// activate armathentication support
#define KRAWALL_SERVER 1

// include common Windows header
#include "win32/config_common.h"

// defines for data directories in Windows
#ifndef DEBUG
#define USER_DATA_DIR  "${APPDATA}/ArmagetronDedicated"
#define SCREENSHOT_DIR "${MYPICTURES}/ArmagetronDedicated"
#else
#define USER_DATA_DIR  "."
#endif

#endif
