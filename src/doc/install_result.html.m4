define(FI,<tr><td><strong>$1</strong></td><td> ($2)</td></tr>)
define(FIS,<strong>$1</strong><br>)

PARAGRAPH([
The contents of the installation directory should be something like this:
])

TABLE([
ifelse(DOCSTYLE_RESULT,unix,
FI(bin,the directory with the executables and scripts),
FI(PROGNAME.exe,the main executable))
FI(models,the directory for the cycle models)
FI(sound,the directory for sound files)
FI(textures,the directory for textures and the icon)
FI(resources,the directory for maps)
FI(doc,the documentation you are just reading)
FI(language,the game's text messages)
ifelse(DOCSTYLE_RESUT,unix,,
[FI(config,static configuration files)]
)
dnl FI(var,game logs and custom configuration)
])

ifelse(DOCSTYLE_RESULT,unix,
[
PARAGRAPH([
The system wide configuration is stored in 
ifelse(DOCSTYLE,web,
[FILE([CONFIGPATH_ETC]) (default) or FILE([CONFIGPATH_NOETC]), depending on your distributors choice.],
[FILE([CONFIGPATH]).],
,)
])
])

PARAGRAPH([
Additionally, if you installed the moviepack/moviesounds
from the ELINK(WEBBASE/addons.html,addons page),
you'll see the two directories
])

FIS(moviepack)
FIS(moviesounds)

define(VAR, ifelse( DOCSTYLE_RESULT,unix, FILE(~/.armagetron/var), the FILE(var) subdirectory))


PARAGRAPH([
containing ONLY files and no subdirectories.
When you have run PROGTITLE, you'll see the additional files
])

<table>
FI([highscores.txt,ladder.txt,won_matches.txt and won_rounds.txt],
score statistics files)
FI(user.cfg,PROGTITLE custom configuration file)
</table>

PARAGRAPH([
Inside VAR,
You may want to create your own 
<a href="config.html#files">configuration files</a>
])

<table>
FI(autoexec.cfg,config file read at every start)
</table>
inside VAR or the 
FILE(config) subdirectory for your 
custom modifications; that way, they don't get overwritten by the defaults the
next time you install an update.

define(CL,<tr><td valign=top width=200><strong>$1</strong> </td><td>$2</td></tr>)

<a name=cl>SUBSECTION(Command line switches)</a>
<table>
CL([-h, +h, --help		] ,[ get help and other available command line options])
CL([-v, +v, --version		], [ print version number ])
CL(,) 
CL([--datadir <Directory>	], [ systemwide data directory; read game data (textures, sounds and text) from this directory. ] )
CL([--userdatadir <Directory>   ], [ user data dir; try to read all game data from this directory first. ])
CL([--configdir <Directory>     ], [ read game configuration (.cfg-files) from this directory.])
CL([--userconfigdir <Directory> ], [ user configuration directory; try to read the configuration from here first. ])
CL([--vardir <Directory>        ], [ save game logs, highscores and user configuration in this directory. ])
CL(,) 
CL([-f, +f, --fullscreen        ], [ start in fullscreen mode])
CL([-w, +w, --window, --windowed], [ start in windowed mode  ])
CL(,) 
ifelse(DOCSTYLE_RESULT,windows,[
CL([+directx, -directx          ], [ enable/disable usage of DirectX for screen initialisation]) 
],[
CL([-d, +d, --daemon            ], [allow the dedicated server to run as a daemon (will not poll for input on stdin)])
])
</table>

PARAGRAPH([
The various directories you can set have the following impact on how the game loads and saves files.
(Usually, you don't have to bother about it; there are reasonable defaults. But if you want to do
some meddling with the data, these options are your ticket.)
])

SUBSUBSECTION([Data])
PARAGRAPH([
Sounds, textures and models will be first looked for in the directory specified with OPTION(--userdatadir). If they are not
found there or if OPTION(--userdatadir) was not used, they are looked for in the directory specified with OPTION(--datadir).
This directory defaults to the current directory if not specified. 
])

SUBSUBSECTION([Configuration])
PARAGRAPH([
(I'll leave out the bits about "if option xxx is not set" from now on )
Configuration files are first looked for in OPTION(--userconfigdir), then OPTION(--userdatadir/config) ( the dir passed to OPTION(--userdatadir), appended with FILE(/config) ),
then OPTION(--configdir) and finally OPTION(--datadir/config).
])

SUBSUBSECTION([Logs and user configuration])
PARAGRAPH([
Log files ( game results, the settings made in the in game menu )  are loaded from OPTION(--vardir), then, if not found there, from OPTION(--userdatadir/var) and finally from OPTION(--datadir/var).
They are only saved in the first of these directories that was specified.
])

ifelse(DOCSTYLE_RESULT, unix,
PARAGRAPH([
[The starter scripts will use the options OPTION([--datadir PREFIX/games/PROGNAME --userdatadir ~/.PROGNAME --configdir CONFIGPATH]). So, your personal configuration will sit in FILE([~.armagetron/var]).
])
])
