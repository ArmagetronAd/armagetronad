dnl some macros to make HTML-creation easier
dnl

include(html.m4.in)

define([PROGTITLE],Armagetron Advanced) 
define([PROGNAMEBASE],armagetronad) 
define([DOCSTYLE],MACOSX)
define([VERSION],bzr)
define([PREFIX],/usr)
define([CONFIGPATH_ETC],/etc/PROGNAME)
define([CONFIGPATH_NOETC],PREFIX/etc/PROGNAME/config)

define([ENABLE_ETC],true)
define([CONFIGPATH],ifelse(ENABLE_ETC,yes,CONFIGPATH_ETC,CONFIGPATH_NOETC))

define([EXE],ifelse(DOCSTYLE,unix,,.exe))
define([LINUX],ifelse(DOCSTYLE,web,Linux))
define([WINDOWS],ifelse(DOCSTYLE,web,Windows))
define([MACOSX],ifelse(DOCSTYLE,web,Mac OS X))

ifelse(DOCSTYLE],web,
[define([PROGNAME],[armagetronad(-dedicated)])]
,
[
ifelse(DEDICATED,,
[define([PROGNAME],[armagetronad-dedicated])],
[define([PROGNAME],[armagetronad])]
)
]
)
