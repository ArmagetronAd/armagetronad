
include(head.html.m4)
define(SECT,bugs)
include(navbar.html.m4)

TITLE(Bug list)

PARAGRAPH([
I've migrated this list to the SourceForge bug list. You can browse it 
dnl ELINK(sourceforge.net/Feature Requests/?group_id=6185,here) and make additions to it.
dnl ELINK(sourceforge.net/bugs/?group_id=6185,here)
ELINK([sourceforge.net/tracker/?atid=657948&group_id=110997&func=browse],here) and make additions to it.
This page is kept for historical reasons.
])

SUBSECTION(Bugs with insufficient information to fix; I'm glad about all hints)
<ul>
<li>Braking and turning at the same time speeds up ( could not reproduce ) </li>
<li>sound does not work sometimes (with winamp in background??)</li>
</ul>

SUBSECTION(Probably unfixable bugs)
<ul>
<li>
If an invalid (i.e. too big) screen resolution is selected,
strange things may happen; anything from crashes to running at a lower
resolution and only showing part of the image. Technically, those are bugs
in the display driver or OS; PROGTITLE can't do more than ask if a screen
mode is OK.
</li>
<li>
Alt-tabbing out of PROGTITLE in fullscreen mode may produce strange behaviour;
if you hit it accidentally, better alt-tab back quickly... Window mode seems
to work flawlessly, however.
</li>
<li>The game system ( starting a match, deciding when someone won, starting new rounds ) is
fundamentally flawed; all changes to it have a high probability of introducing nasty new bugs. 
All following bugs are caused by this mess. I'll give it a complete workover for 0.3.0, if I ever
get this far.</li>
<li>( Mixed human/AI teams do not work very well )</li>
</ul>


SUBSECTION(Probably fixed bugs)
<ul>
<li>Network game with holes in walls: holes disappear when they get blown into a cycle's current
wall as soon as it takes its next turn ( client only effect )</li>
<li>two human teams with only AI players on will keep playing on a dedicated server</li>
<li>crash when opening the score-view after the following actions:
server started<br>
client with one player connects<br>
client with two players connects, one of them joins the team from the other client<br>
</li>
<li>Inactive players get accounted as team members</li>
<li>the dedicated server won't go daemon(it should notice by itself that
it's standard io is not the console)</li>
<li>Strange camera behaviour; I only observed it myself when the frame rate is
very high (200 and up)</li>
<li>Invisible bikes on the Voodoo 2 under Windows (Jacob Aaron Barandes),
caused by broken implementation of display lists. Other Voodoo 2 systems 
show different kind of trouble...</li>
<li>RADEON: no valid screen mode found. Implement desktop colour depth and ignore errors. (Nathan Parslow <bruce@linuxgamers.net>)</li>
<li>Seemingly random disconnections (network object IDs get assigned
incorrectly every once in a while)</li>
<li>server browser does not get the complete list from master</li>
<li>Server browser crashes when an unreachable server under the cursor is
removed</li>
<li>Disconnected state is set when connecting to a server, disconnecting and
starting an own server</li>
<li>Disconnected state is set when the last guest quits on an own server</li>
<li>Chat state does not work in server mode</li>
<li>network mode without dedicated server: rounds are reseted when someone connects</li>
<li>network mode: use special multiplayer-settings rather than special single-player settings</li>
<li>network mode: player changes are not transmitted immediately</li>
<li>explosion sounds are often not played</li>
</ul>




SUBSECTION(Fixed bugs)

<ul>
<li>PROGTITLE exits with a message like "Point (569.977, 269.095) does not have a face in direction (0, 2.05167) Please Send A Bug Report!"; That means the
internal data structures are corrupt and don't allow another wall to be drawn. Maybe I should just rebuild the structures when that happens? Hmm...</li> 
<li>Problems when logging in on a server with a game already running (Managed to reproduce it finally)</li>
<li>AI players drove out through the rim wall and came back alive</li>
<li> with fast video cards or "Express Finish": Champion of a
match was declared in an additional round </li>
<li>when changing the screen mode in mid-game, the cycles became invisible</li>
<li>The textured floor did not work on all known ATI chips (from Rage to Radeon) and probably all S3 chips; (OK, I know now
what would have to be done. But I'm too lazy currently)</li>
<li>Seemingly random connection terminations; caused by packet loss.</li>
<li> Matches on a dedicated server started in round 2</li>
<li> sometimes: crashes with "... left grid"-message </li>
<li> the mouse cursor stays visible on some Linux systems (VooDoo 3000
 AGP) </li>
<li>the control over the own bike is poor in network mode (about 99% perfect now)</li>
<li>When you play the highscore hunt on a dedicated server and someone
enters, you got accounted one won round on your score, and the
multiplayer settings were not restored.</li>

<li>camera hiccups</li>

<li> ALT-Tabbing out of PROGTITLE for Windows causes BAAAD lockups.</li>

<li>The network mode is still not tested very well, but there were no
 BAD crashes the last couple of hours. One problem that is hard to get
 away: Making many fast turns confuses the client for a while;
 driving straight again makes it catch on after a short while. Fixing
 this would require a change in the network protocol, and I promised
 not to do that until after version 0.1. (I mainly got it fixed now.)</li>

<li>player Coors are wrong in network play sometimes</li>

<li>does not exit cleanly on some configurations</li>

<li>the server exits every now and then, even with players on it (some
crashes, but most of the time the cpu time limit is hit)</li>

<li>It seems to be impossible for a second user to join a network game
 if there are >8 AI players; yet, I couldn't reproduce this bug.</li>

<li>Texture files MUST be in RGBA - 32bit/pixel mode. (corrected. They
 may now be in RGBA or RGB format.)</li>

<li>logging in to the server often fails with timeouts</li>

<li>sometimes, there are "ghost bikes".</li>

<li>Some generic hangups (an esp. nasty one affected even the server).</li>

<li>in "Fast Finish" mode, there are hangups when you lose.</li>

<li>Chatting during the game break killed you immediately in the next game.</li>

<li>It was possible do drive through walls:
<ul>
<li>drive straight to then</li>
<li>turn EXACTLY on the wall</li>
<li>turn the other direction</li>
</ul>
I accidently made it through the outer wall that way.
Haven't seen such things happen lately, so I consider it fixed.</li>

<li>The network mode had various problems: random crashes, players
getting killed just because they took a turn, walls being temporarily
invisible.</li>

<li>The Linux version crashes on exit. (It's a boomerang bug: I think it
 will come back again. Caused by C++ destructors of static objects
 called in the wrong order.) </li>

<li> The sky is supposed to have a foggy look (alpha effect).
 with Win9x VooDoo or TNT hardware rendering however, there seem to exist 
 only the alpha values 0 (=fully transparent) or 1 (=opaque). 
 Linux version and Win9x software rendering are not affected.
 What am I doing wrong? Please send me a small OpenGL program
 that uses correct alpha blending so I can correct this.
 (Stupid me. I explicitly ordered OpenGL to store the textures with
  one bit alpha; Mesa ignored that.)</li>

<li>The timer does funny things sometimes (causing you to crash into
 yourself by just taking an turn)</li>

<li>The far edges of the grid are not displayed at match start.</li>
</ul>

SUBSECTION(Those are NOT bugs:)
<ul>

<li>in "Freestyle" game mode, there game does not stop until you are dead.</li>

<li>"Z-Trick" and "Infinity" don't work on your PC</li>

<li>In a network game, it sometimes <strong>seems</strong> like bikes can pass through walls</li>

</ul>

include(sig.m4)
include(navbar.html.m4)
</body>
</html>







