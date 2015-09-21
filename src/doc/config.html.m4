include(head.html.m4)
define(SECT,config)
include(navbar.html.m4)

TITLE(Configuration)

All the normal user settings of PROGTITLE are configurable in
the menu system. Since it is quite easy to get lost, here is a
(not always up-to-date) map of all submenus (the menu items have
an auto help display below if you don't touch any keys for some seconds,
 so to let you keep the overview here, I omit the simple items):

define(CI,<CODE>[$1]</CODE>)

define(MI,<li><strong>$1</strong> ($2)</li>)

SUBSUBSECTION(Main Menu)
<ul>
MI(Game,start and setup a game)
<ul>
MI(Local Game,Starts a local game)
MI(Network Game,Connects to or creates a network game)
MI(Game Setup,Sets your favourite single player game mode: number of AI opponents, game speed, arena size)
</ul>
MI(Player Setup,Player customisation: [keyboard input, camera setup, multiplayer
mode on one computer])
<ul>
MI(Player 1-4,the settings for this player)
<ul>
MI(Input Configuration,keyboard and mouse setup)
MI(Camera Input Configuration,keyboard and mouse setup for camera controls)
MI(Camera Setup,camera mode options)
MI(Instant Chat,things you can say with one keystroke)
</ul>
MI(Assign Viewports To Players,which player sees himself on which part of
the screen)
</ul>
MI(System Setup, Configure sound, graphics and preferences here])
<ul>
MI(Display Settings,screen resolution and graphic details/preferences)
<ul>
MI(Screen Mode,resolution and windowed/fullscreen selection)
MI(Preferences,things that depend more on your personal taste)
MI(Detail Settings,things that affect visual quality and depend on your
system's OpenGL power)
MI(Performance Tweaks,[settings that increase graphics speed, but
may not work on your system])
</ul>
MI(Sound Settings,sound quality)
MI(Misc Stuff,things that did not fit anywhere else: [Moviepack, console text
output, menu wrap option and global keyboard configuration])
<ul>
MI(Global Keyboard Configuration,[keyboard setup for player-independent functions (console, scores)])
MI(Language settings,[Choose the language of PROGTITLE here])
</ul>
</ul>
</ul>

define(SI,<li>$1</li>)

<a name=new>SUBSECTION(Game Rule Settings)</a>
PARAGRAPH([
Some of the maybe less obvious game settings in "Game Setup" available since version 0.2 are explained here:
<ul>
SI([The AIs come in various intelligence grades. Select it in the "AI IQ" menu item.])
SI([Explosions can blow away walls. Select in "Blast Radius" how much destruction they bring.])
SI([Cycle walls can be of finite length (a la snake). Choose in "Wall Length" how long they should be.])
SI([The delay between the a cycle explosion and the disappearing of its wall can be configured in "Wall Delay".])
SI([Arena size and overall game speed can be adjusted in "Arena Size" and "Speed". Note that the setting here is logarithmic; that
means that by increasing the value by 2, you double the arena size/game speed. by increasing the value by 6, you multiply the size/speed by a factor of 8.])
</ul>
Only important for multiplayer games:
<ul>
SI([Limits on the number of teams and the number of players per team can be set in the "Min/Max teams/players per team" settings.])
SI([You can control team ballance with the "Max imbalance" and "Ballance with AIs" settings.])
</ul>
])

SECTION(Debug Recording,recording)
PARAGRAPH([
PROGTITLE supports recording whole program sessions for debugging and performance tuning.
])
SUBSECTION(Usage)
PARAGRAPH([
To record a program session, use the command line parameter CI(--record &lt;filename&gt;);
this will store the recording in the file CI(&lt;filename&gt;) in the current directory.
A suggested file extension would be CI(.rec).
])
PARAGRAPH([
To playback a recording, use the command line parameter CI(--playback &lt;filename&gt;).
If you want to skip ahead to the interesting part,
you can do so by giving the additional parameter CI(--fastforward &lt;time&gt;).
While playing back a client session, the current time will always be 
displayed in the upper right corner of the screen.
])
PARAGRAPH([
During playback, input from the keyboard is not disabled.
You can use it safely for nonessential input,
like controlling the camera or toggling the score display.
However, issuing cycle turn commands is likely to break the playback.
It is possible to combine both playback and recording arguments on the command line;
this gives you rudimentary recording editing possibilities.
])
PARAGRAPH([
The command line switch CI(--benchmark) plays the recording 
back frame by frame as it was recorded and give performance statistics at the end of the run. 
Without this option, the playback will try to match the real speed.
])
ifelse(DOCSTYLE,linux,,[
PARAGRAPH([
On MS Windows, to give the PROGTITLE command line arguments, open up a command console (aka dos box)
and type (if you have installed PROGTITLE into E:\Program Files\PROGNAMEBASE):
<pre>
E:\
cd E:\Program Files\PROGNAMEBASE
./PROGNAMEBASE &lt;command line arguments&gt;
</pre>
But you don't have to go through that hassle; there are three start menu entries that record
to a file on your desktop and play it back normally or in benchmarking mode.
])
])
SUBSECTION(Background)
PARAGRAPH([
Recording works by storing all external input
(keypresses, time measurements, file reads and network traffic)
in a file and feeding the stored input to the program on playback.
Due to the purpose and the implementation, there are limitations: 
It is unlikely that a recording done with one version will play back on another version.
It often happens that even with the same version, different builds
(Windows vs. Linux, different GCC versions used to compile, PC vs. Mac, Debug vs. Optimized)
produce incompatible playbacks because of subtle
differences in the way numbers are treated by different processors,
compilers and operating systems.
])
SUBSECTION(Protect Your Passwords)
PARAGRAPH([
Since all network traffic is logged and the ingame admin password is currently sent unencrypred,
it is not a good idea to publish server recordings where you or anyone else logs in as admin.
The same holds for a client session recording where you log in anywhere; remember that all your
keypresses are recorded without exception.
])
SUBSECTION(Primary Use: Bug Reports)
PARAGRAPH([
Sometimes, when you report a bug, the team may ask you do send a recording of
the bug happening.
It is then most useful if you do your recording while you have
VSYNC enabled in your video driver settings
(so your framerate and the rate of recording is limited by your video refresh,
which makes the recording smaller and faster to fast forward through) and spark rendering disabled.
Please try to make the bug happen as soon as possible. 
Because of the limitations of the recordings, we'll also
need to know the exact version and build you are using.
A screenshot or even a series of screenshots edited by you that show us what you think is
wrong also helps greatly;
if you make them while playing back your recording,
we have the added benefit that we see the timestamp and know where to fast forward to.
])
SUBSECTION(Secondary Use: Demo Recording)
PARAGRAPH([
If you want to use a recording to show off,
it is best to do record it on the client in a network
session even if you just want to demonstrate your superior AI killing ability.
This increases the chances for compatibility greatly because all game physics decisions are
made on the server and stored in the recording in the form of the network traffic.
Keep the limitations in mind, though;
it is not a bug if this recording does not play back
on anything but your own machine and the exact version it was recorded with.
])


SECTION(Advanced Configuration,files)

PARAGRAPH([
Take a look at the files
FILE(settings.cfg) and FILE(settings_dedicated.cfg) for the dedicated server
(best save all your changes to 
FILE(autoexec.cfg) if you don't want the next
release of PROGTITLE to overwrite them). They contain a lot of settings
inaccessible from the menu system. All relevant settings are synced from the
server to the connected clients.
You can place comments preceeded by <CODE>#</CODE> anywhere in these files.
])

PARAGRAPH([
The settings that affect only the visual appearance (<CODE>FLOOR_*</CODE> ...)
are not transferred; It stays a matter of taste if you want the floor
red or yellow.
])

SUBSECTION(Game Physics)
SUBSUBSECTION(Speed)
PARAGRAPH([
The game is too slow for you? Then 
play a bit with <CODE>CYCLE_SPEED</CODE> and all the other
nice variables and see what happens.
Not all possible setting items are included in these files, you can add everything
you find in LINK(commands.html,this unsorted document).
])

PARAGRAPH([
The base acceleration in an open field is <pre>(CYCLE_SPEED-current speed) * f</pre>
where f is
CI(CYCLE_SPEED_DECAY_BELOW) if the current speed is smaller than CI(CYCLE_SPEED) resp.
CI(CYCLE_SPEED_DECAY_ABOVE) if it is bigger. The effect is that the cycle's speed will
approach CI(CYCLE_SPEED) over time. The bigger the constants, the faster the approach.
])

PARAGRAPH([
The acceleration caused by the wall at distance <CODE>D</CODE> is
<pre>
acceleration=CYCLE_ACCEL / (CYCLE_ACCEL_OFFSET + D)-
             CYCLE_ACCEL / (CYCLE_ACCEL_OFFSET + CYCLE_WALL_NEAR)
</pre>
if the wall is closer than <CODE>CYCLE_WALL_NEAR</CODE>. 
the stuff in the second
line just makes sure that the acceleration is 0 if 
<CODE>D=CYCLE_WALL_NEAR</CODE>, so there are no "jumps".
Depending on the nature of the wall (whether it belongs to you, your teammate, 
your enemy or is the arena rim), the acceleration gets multiplied with 
CI(CYCLE_ACCEL_SELF), CI(_TEAM) resp. CI(_ENEMY) or CI(_RIM). If your cycle
is stuck between one of your walls and a wall of any other type, your total
acceleration by walls gets multiplied by CI(CYCLE_ACCEL_SLINGSHOT).
])

PARAGRAPH([
Furthermore, to make the overall speed feeling configurable without disturbing the
tuning of the values with respect to each other, CI(CYCLE_SPEED) and CI(CYCLE_ACCEL)
are multiplied by exp(CI(SPEED_FACTOR)*ln(2)/2), where CI(SPEED_FACTOR) is the 
value accessible as "Game Speed" in the game settings menu.
On single player games on a dedicated server, the value of CI(SP_SPEED_FACTOR)
is taken instead.
]) 

SUBSUBSECTION(Turning)
PARAGRAPH([
Cycles are not allowed to make arbitrarily fast turns; mostly, this is to take away
the advantage the AI opponents would have over you, and to avoid flooding a server
with fast turn commands. The minimum time between turns is determined by CI(CYCLE_DELAY).
])
PARAGRAPH([
It may be desirable to modify the minimum time between turns based on the cycle's speed;
that's what the setting CI(CYCLE_DELAY_TIMEBASED) does. If it is at the default value
of 1, the cycle delay works as described in the above paragraph. If it is increased,
the time between turns gets longer the faster you go, if it is decreased, the time between
turns gets shorter the faster you go. At 0, two fast consecutive turns will approximately
have the same distance from each other.
])
PARAGRAPH([
At every turn, your cycle loses a bit of speed; how much is determined by 
CI(CYCLE_TURN_SPEED_FACTOR). At every turn, your speed gets multiplied with this value.
Set it to 1 to disable the turn slowdown.
])

SUBSUBSECTION(Rubber)
PARAGRAPH([
The <CODE>CYCLE_RUBBER</CODE> setting is a niceness thing, especially useful
in an internet game where player prediction is poor: if you
hit a wall, you are not directly deleted; you are stopped for a
short time and your supply of "rubber" is decreased. If you manage
to turn fast enough, you won't die. Rubber has three aspects: how close
you can get to walls, how fast you do so, and how long long you survive.])
PARAGRAPH([
How close you can get to a wall is determined by the CI(CYCLE_RUBBER_MINDISTANCE)
family of settings. All contributions are summed up. The basic one is
CI(CYCLE_RUBBER_MINDISTANCE) itself: you won't be able to get closer to a wall
than that. CI(CYCLE_RUBBER_MINDISTANCE_RATIO) is there to avoid trouble with inexact
internal calculations: The lenght of the wall in front of you is multiplied by
this value, then added to the effect of CI(CYCLE_RUBBER_MINDISTANCE). The next
two contributions influence gameplay: CI(CYCLE_RUBBER_MINDISTANCE_RESERVOIR) gets
multiplied with the rubber you have left to burn. If your rubber meter
shows no rubber used, the effect is in full force; if your rubber meter is almost full,
there is virtually no effect. CI(CYCLE_RUBBER_MINDISTANCE_RESERVOIR) makes grinds
where you use up all your rubber deeper. CI(CYCLE_RUBBER_MINDISTANCE_UNPREPARED) gets
added if your last turn was only a short time ago; if you turn right into a wall, it is
at full effect; if your last turn was more than CI(CYCLE_RUBBER_MINDISTANCE_PREPARATION)
seconds ago, the effect gets weakened approximately inversely proportional to the time
since the last turn.
])
PARAGRAPH([
All these CI(MINDISTANCE) settings get overridden if your last turn was already very close
to a wall (like the maneuvers known as 180s and adjusts), your cycle is allowed to
get closer to the wall by the factor CI(CYCLE_RUBBER_MINADJUST) before they kick in.
])
PARAGRAPH([
How fast you are allowed to approach a wall is determined by CI(CYCLE_RUBBER_SPEED).
It's a logarithmic speed; you will be able to make half the distance to a wall
in no less than T=ln(2)/CI(CYCLE_RUBBER_SPEED) seconds, make the next quarter
in another T seconds, and so on; you never reach the wall completely.
Rubber usage gets activated only if your current speed is larger than the speed determined
by this (&lt;distance_to_wall&gt;*CI(CYCLE_RUBBER_SPEED)) at every moment).
On 0.2.7.0 and earlier, the corresponding code was dependant on your framerate;
the new code behaves approximately like the old code at CI(CYCLE_RUBBER_SPEED)/ln(2) fps.
ln(2) is about 0.7.
])
PARAGRAPH([
The main variable that determines how long you survive is
CI(CYCLE_RUBBER). In network games, your ping (in seconds) is multiplied by
CI(CYCLE_PING_RUBBER) and added on top of that. How much rubber is used
is based on the distance you would have covered had the rubber not stopped
you or slowed you down. If you use up all your rubber, it loses its effect
and usually you die. Your reservoir gets refilled slowly over a timescale
determined by CI(CYCLE_RUBBER_TIME) (defaults to 10 seconds).
])
PARAGRAPH([
Since rubber is based on distance, it is used up faster if you go faster.
You can change that with CI(CYCLE_RUBBER_TIMEBASED); set it to 1 to base
the rubber usage on time instead. You can also set it to 2 to make rubber
even more effective as you go faster, or -1 to make it more ineffective
than usual when you go fast. Intermediate values are also allowed.
])
PARAGRAPH([
How would you call it if you drive parallel close to a wall, then turn towards it?
I'd call it stupid. To punish this stupidity, the rubber efficency can be modified
for a short time after each turn (so more rubber is used than usual).
To activate this, set CI(CYCLE_RUBBER_DELAY)
to a positive value. For CI(CYCLE_RUBBER_DELAY)*CI(CYCLE_DELAY) seconds after every
turn, the rubber efficiency gets multiplied with CI(CYCLE_RUBBER_DELAY_BONUS). Values
smaller than 1 will increase rubber usage, a value of zero deactivates the rubber
protection completely for that time.
])

PARAGRAPH([
Summary: You're certainly confused about all these settings for such a simple feature;
the most important ones to play with first are CI(CYCLE_RUBBER), CI(CYCLE_RUBBER_SPEED)
(set it to something low to feel its effect) and CI(CYCLE_RUBBER_TIME). They determine
how long you survive, how fast you grind, and how fast your rubber replenishes (in that
order). An alternative description can be found in 
ELINK([forums.armagetronad.net/viewtopic.php?t=1774],[this forum thread]).
])

SUBSECTION(Camera Modes)

PARAGRAPH([
A special feature are the <CODE>CAMERA_FORBID_*</CODE> settings; 
if you think for
example that using the free floating camera is cheating, simply
set <CODE>CAMERA_FORBID_FREE</CODE> to 1 on your server; 
none of the clients will
then be able to use the free camera. (Of course, it is possible
for a modified client to cheat at this point, using the forbidden
camera perspectives anyway.) Try a match with all but the internal
camera disabled :-)
])

SUBSECTION(Maps)
PARAGRAPH([
New in 0.2.8 is the possibility to select an arena to fight in. It is still a bit experimental
(not because the arena code itself is buggy, but because the AI opponents have not been adapted),
and to stress this, it is not yet possible to choose the map in the menu. Also, apologies
for the
quite strict rules server administrators need to follow (see below), we're working on automating
some of the administrative overhead.
])

PARAGRAPH([
To change the map, set the CI(MAP_FILE) variable to the path to the map. Maps are searched in the
LINK([config.html#resource],resource folders). There are example setting lines for
all the included maps in settings.cfg, you just have to uncomment the appropriate lines.
])

PARAGRAPH([
If you want to override a map's number of driving directions, you can do so with CI(ARENA_AXES).
])

PARAGRAPH([
If you want to create your own map, you should read the LINK(Howto-Maps.txt, Maps HOWTO).
])

SUBSUBSECTION(Automatic downloading)
PARAGRAPH([
Maps will be automatically downloaded from the internet if they are not available locally. First,
the URI given in the setting CI(MAP_URI) is tried; if that fails, the URI 
CI(RESOURCE_REPOSITORY/MAP_FILE) is used.
])

PARAGRAPH([
<strong>Important for server administrators:</strong> MAP_FILE needs to follow strict rules since
it determines the position the map will be stored under for all clients that connect to your
server. Not following these simple guidelines will mess up your clients' automatic resource
directory and they'll blame the development team for not providing a mechanism that
magically puts maps into the right place (and they are right with that, but that's
another stroy). Anyway, the guidelines: CI(MAP_FILE) should be of the form
AuthorName/[[SubDirectory/]]ResourceName-VersionNumber.aamap.xml, and usually, the attributes
of the Map tag will tell you what you should use for AuthorName, ResourceName and
VersionNumber and, if the author chooses to be organized, the optional SubDirectory part as well.
If the map you want to use does not give you
enough information to uniquely derive CI(MAP_FILE) from it, ask the author. There is a python
script called CI(sortresources.py) that automates putting the resource files into the right place.
])

PARAGRAPH([Also, it should be noted that maps <strong>need</strong> to be versioned:
no two files with different content and the same VersionNumber must exist. Ever. Really confused
clients would be the result.
])

SECTION(The Console,console)
PARAGRAPH([
All the settings you alter permanently in the .cfg-files can be
tested temporarily if you enter them at the console. To enter a
line to the console, you have to bind a key to it 
(in "Misc Stuff/Global Keyboard Configuration") and press it, of course.
The line you enter will be interpreted just as if it was read from
a config file.
if you type in an incomplete
name, you'll get a list of settings that contain that name. Just typing
the name of a setting will print the current value.
<br>
If you run a <a href=network.html#dedicated>dedicated server</a>, 
everything you type will be considered console input.  
])

SUBSUBSECTION(Special Console Commands)
define(CC,<tr><td valign=top><CODE>$1 </CODE></td><td>$2</td></tr>)
<CODE>x</CODE> always is a real number, <CODE>n</CODE> an integral number,
<CODE>b</CODE> a boolean value (0=false,1=true) and <CODE>s</CODE> a string
(all the rest of the line will be read).
<table>
CC(START_NEW_MATCH,[reset the scores and start a new match in the next round])
CC(DEDICATED_IDLE x,[Only used by the dedicated server: after running for x hours, the server takes the next opportunity (when no one is online) to quit.])
CC(QUIT or EXIT,[shuts the server down])
CC(KICK s,[Kicks player s from the server])
CC(CENTER_MESSAGE/CONSOLE_MESSAGE s,[Prints a message for all connected clients at the
center of the screen/on the console])
CC(SAY,[Dedicated server only: let the server administrator say something])
</table>
SUBSUBSECTION(Other Console Commands)
<table>
CC(CYCLE_SPEED x, basic cycle speed (m/s))
CC(CYCLE_START_SPEED x,cycle speed at startup)
CC(CYCLE_ACCEL x,acceleration multiplicator)
CC(CYCLE_ACCEL_OFFSET x,acceleration offset (higher means lower acceleration))
CC(CYCLE_DELAY x,minimum time between turns)
CC(CYCLE_WALL_NEAR x,when is a wall near?)
CC(CYCLE_SOUND_SPEED x,sound speed divisor; the speed at which the cycle sound is played at normal speed) 
CC(CYCLE_BRAKE x,brake strength)
CC(CYCLE_RUBBER x,niceness when hitting a wall)
CC(CYCLE_PING_RUBBER x,[niceness when hitting a wall, influence of your ping])
CC(FLOOR_RED/GREEN/BLUE x,floor colour (without moviepack))
CC(FLOOR_MIRROR_INT x,floor mirror intensity(if enabled))
CC(GRID_SIZE x,distance of the grid lines)
CC(CAMERA_FORBID_SMART b,forbid smart camera)
CC(CAMERA_FORBID_IN b,forbid internal camera)
CC(CAMERA_FORBID_FREE b,forbid free camera)
CC(CAMERA_FORBID_FOLLOW b,forbid fixed external camera)
CC
CC(SCORE_WIN n,points you gain for being last one alive)
CC(SCORE_SUICIDE n,points you gain for every stupid death 
(race into the rim/your own wall); should be negative)
CC(SCORE_KILL n,points you gain for everyone racing into your wall)
CC(SCORE_DIE n,points you gain for every time you race into someone's wall (should be negative))
CC
CC(LIMIT_SCORE n,score limit (all limits for one match))
CC(LIMIT_ROUNDS n,maximum number of rounds to play)
CC(LIMIT_TIME n,maximum time (in minutes))
CC(MESSAGE_OF_DAY_1/2/3/4 s,message lines sent to the clients upon connection)
CC
CC(COLOR_STRINGS b,draw strings in colour?)
</table>

PARAGRAPH(
<a name=sphh>Settings for single player highscore hunt on this server:</a>
<table>
CC(SP_SCORE_WIN n,points you gain for being last one alive)
CC(SP_LIMIT_ROUNDS n,max number of rounds to play)
CC(SP_LIMIT_TIME n,max time (in minutes))
CC(SP_AIS n,number of opponents you will face)
</table>
)

PARAGRAPH(
Ladder league constants:
<table>
CC(LADDER_PERCENT_BET x,percentage of your score to be put in the pot)
CC(LADDER_MIN_BET x,minimum credits to be put in the pot)
CC(LADDER_TAX x,percentage the IRS takes from the pot)
CC(LADDER_LOSE_PERCENT_ON_LOAD x,you lose this percentage of your
score every time the server is restarted)
CC(LADDER_LOSE_MIN_ON_LOAD x,but minimum this value)
CC(LADDER_GAIN_EXTRA x,the winner gets his ping+ping charity
(in seconds) times this value extra)
</table>
)

SECTION(Complete List)
PARAGRAPH([
A complete list of all available commands is available LINK(commands.html,here).
])

SECTION(Resource Path,resource)
PARAGRAPH([
The directories searched for resources are the explicit resource path given by 
the CI(--resourcedir) command line
argument and the FILE(resource) subdirectories of the systemwide data directories
 and user data directories. The 
FILE(included) subdirectories of these resource directories are searched as well, but you should
not put files in there, they are reserved for resources we include in our distribution. 
<br>
Lastly, there is
a directory reserved for resources automatically downloaded from the network; usually, it is the
directory called FILE(resource/automatic) under the user data directory, but falls back to
the same subdirectory of the system data directory and can be overridden by the 
CI(--autoresourcedir) command line argument. The FILE(automatic) subdirectory of the explicit
resource path given with CI(--resourcedir) overrides the default automatic resource
path as well.
])


include(sig.m4)
include(navbar.html.m4)
</body>
</html>


