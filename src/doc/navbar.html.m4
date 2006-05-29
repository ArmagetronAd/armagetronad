dnl<hr width="100%">

<table width="90%" align=center>
<tr>

define(ELEMENT,dnl
<td align=center width="$3%">
[ifelse(SECT,$1,
<strong>$2</strong> dnl
,
<a href="$1.html" target="_top">$2</a>)]
</td>
)



ifelse(DOCSTYLE,web,
[
 define([show_windows],yes)
 define([show_unix],yes)
 define([show_mac],yes)
]
,
[
 define([show_windows],no)
 define([show_unix],no)
 define([show_mac],no)
]
)

ifelse(DOCSTYLE,windows,[define([show_windows],yes)] )
ifelse(DOCSTYLE,macosx,[define([show_mac],yes)] )
ifelse(DOCSTYLE,unix,[define([show_unix],yes)] )

ELEMENT(index,First Start,15)
ifelse(show_windows,yes,[ ELEMENT(install_windows,[WINDOWS Installation],15) ] )
ifelse(show_mac,yes,    [ ELEMENT(install_macosx,[MACOSX Installation],15)   ] )
ifelse(show_unix,yes,   [ ELEMENT(install_linux,[LINUX Installation],15)     ] )
ELEMENT(network,Network Play,15)
ELEMENT(config,Configuration,15)
ELEMENT(faq,FAQ,10)
dnl ELEMENT(bugs,Bugs,10)
dnl ELEMENT(todo,Todo List,10)
dnl ELEMENT(changelog,Changelog,15)
ifelse(DOCSTYLE,windows,,ELEMENT(compile,Redistribution,15))

</tr>
</table>
dnl<hr width="100%">
dnl<br>

