include(head.html.m4)
define(SECT,index)
include(navbar.html.m4)

TITLE(First Start)

SUBSECTION([WARNING: (READ IT! it's NOT legalese, I mean it!)])

PARAGRAPH([
Some people react on flashing lights in a very unpleasant manner:
they experience epileptic seizures. Even if you did never have such
a seizure, you may still be affected by those induced seizures. 
Since PROGTITLE contains rhythmic flashing lights (the grid you race
over...), there is a higher risk of induced seizures than in normal
computer games. FOR YOUR OWN SAFETY, do NOT play PROGTITLE on a wide TV
screen or in a dark room and always keep a good eye distance to
your monitor. WE DO NOT FEEL RESPONSIBLE for anything that might happen
to you while playing PROGTITLE, even if you do respect these advises
(See legal stuff below). 
])

SUBSECTION(How to play)

PARAGRAPH([
The rules are simple: you ride a lightcycle, a kind of motorbike that
can only turn 90 degrees at a time, leaves a wall behind and cannot be
stopped. Avoid running into a wall while trying to make your opponent run into a wall.
Just in case you do not know: this idea is best known from the Disney
movie "Tron" from 1982. However, that's not the origin of the game idea:
The earliest arcade game of that type was 
ELINK(www.klov.com/B/Blockade.html,Blockade) from 1976, the first known home versions
ELINK([en.wikipedia.org/wiki/Snafu_%28video_game%29],SNAFU) on the Intellivision from 1981 and
ELINK(www.atarihq.com/reviews/2600/surround.html,Surround) for the Atari 2600.
])

PARAGRAPH([
Acceleration is possible by driving close to player walls, be it your own wall or the
wall of your enemy. Every time you make a turn, you lose a little bit of your speed, so don't be 
too nervous; speed is your only resource against other players!
])

SUBSECTION(Quick start)
PARAGRAPH([
First, you'll be confronted with the selection of your favourite language. 
Use CURSOR UP/DOWN to select, then ENTER/RETURN to accept. Next, there is a quick
and simplified setup screen. use CURSOR UP/DOWN to navigate the entries, CURSOR LEFT/RIGHT
to make changes to selection items and use standard editing controls to change your name.
If you're happy with the choices, press ENTER on any of the two 'Accept' menu items.
You will be tossed into a training match against a single AI opponent with reduced speed
where you can learn the keybindings.
])
PARAGRAPH([
Reading this, you probably already went through those steps. You can repeat the simplified
setup in the menu "System Setup"/"Misc Stuff"/"Redo First Setup" and pick different languages
in "System Setup"/"Misc Stuff"/"Language Settings".])

SUBSECTION([Additional keyboard layout])

PARAGRAPH([
<table>
<tr><td>show scores            </td><td>: TAB</td></tr>
<tr><td>scroll message console </td><td>: page up/down</td></tr>
<tr><td>toggle fullscreen	   </td><td>: n</td></tr>
<tr><td></td></tr>
<tr><td width = 200 >pauses/unpauses the game</td><td>: p</td></tr> 
<tr><td>quit to the game menu</td><td>: q/ESC</td></tr>
</table>
])

PARAGRAPH([
SHIFT-ESC is the "boss key" and quits PROGTITLE as fast as possible.
])

SUBSUBSECTION(Menu navigation:)

PARAGRAPH([
<table>
<tr><td>Move up one item</td><td>cursor up</td></tr>
<tr><td>Move down one item</td><td>cursor down</td></tr>
<tr><td>Change item</td><td>cursor left/right</td></tr>
<tr><td width = 200>Enter Submenu/Activate item</td><td>return/enter/space</td></tr>
<tr><td>Leave Menu</td><td>ESC</td></tr>
</table>
])

SUBSUBSECTION([change controls (in "Player Menu"/"Player x"/"(Camera) Input Configuration" and "System Setup"/"Misc Stuff"/"Global Keyboard Configuration"):])

PARAGRAPH([
Press STRONG(enter) or STRONG(space) on the control you wish to change, then press
the key/mousebutton you want to assign to that control. Moving the
mouse also works. 
<br>
Press STRONG(delete) or STRONG(backspace) to reset the control binds for the selected
functions.
])

PARAGRAPH([
After your first matches, you should  go into the "Player Menu", set your name,
your keyboard and mouse configuration. Then, get some friends together and try
the multiplayer modes!
])

SUBSECTION([Feedback, support, keeping up to date:])
PARAGRAPH([
Visit the ELINK([www.armagetronad.net/],main webpage) for up to date information
about PROGTITLE and the latest stable builds. Check out the 
ELINK([forums.armagetronad.net/],Forums) for keeping in touch with
current development and venting your anger that we broke your favorite feature
in the current release. Check the ELINK([wiki.armagetronad.net],Wiki) for community maintained documentation, the pages you're reading right now only scratch the surface. Visit the 
ELINK(sourceforge.net/projects/armagetronad/,SourceForge project page) to get
informed by email or RSS when a new release is out. Most of our current development is managed
on ELINK(launchpad.net/armagetronad,Launchpad). Get the latest
snapshot builds in various flavors from ELINK(sourceforge.net/projects/armagetronad/files/snapshots/,the SourceForge file release system).
])

SECTION(Credits:)

Directly from the AUTHORS file:
<pre>
include(../../AUTHORS)
</pre>

Additional thanks go out to:

<ul>
<li>Chaz Guarino (MAILMANGLE(Clockwiser,aol,com)) for extracting the sounds from
the movie in high quality.</li>

(Both the moviepack and the movie sounds are available for download
separately on the old ELINK(WEBBASE/addons.html,Addons page)
because of their size and copyright issues)

<li>Max Rudberg(MAILMANGLE(max_08,mac,com)) for the Mac icon</li>
 
<li>All the other fans that sent their suggestions and comments, especially eRVee and Andrew Macks!
</ul>

SUBSECTION([Legal stuff: license and warranty])

PARAGRAPH([
This program is free software; you can redistribute it and/or
modify it under the terms of the 
LINK(COPYING.txt,GNU General Public License)
as published by the Free Software Foundation, version 2.
])

PARAGRAPH([
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
])

PARAGRAPH([
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
])

ifelse(DOCSTYLE,windows,
[
PARAGRAPH([
For you, this means that you are absolutely free to redistribute the binary and source
archives of this game in any way you like: send them to your friends, mirror them on your FTP server or
put them on magazine cover CDs.
])
]
)

dnl define(NOSIG,yes)

include(sig.m4)
include(navbar.html.m4)
</body>
</html>

