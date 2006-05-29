dnl<hr width="100%">

<table width="90%" align=center>
<tr>

define(ELEMENT,dnl
<td align=center width="$3%">
[ifelse(SECTION,$1,
<strong>$2</strong> dnl
,
<a href="$1.html" target="_top">$2</a>)]
</td>
)

ELEMENT(index,Introduction,25)
ELEMENT(lower,Layer One,25)
ELEMENT(middle,Layer Two,25)
ELEMENT(upper,Layer Three,25)

</tr>
</table>
dnl<hr width="100%">
dnl<br>

