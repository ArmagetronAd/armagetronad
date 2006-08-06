include(head.html.m4)
define(SECT,network)
include(navbar.html.m4)

define(SI,<CODE>[$1]</CODE>)
define(CI,<CODE>[$1]</CODE>)

<a name=np>TITLE(Network Play)</a>

PARAGRAPH([
You can play PROGTITLE over a LAN or the Internet. It uses the UDP 
connectionless communication mode of the IP protocol, so make sure 
you have TCP/IP installed and activated if you experience problems.
])

SECTION(Connecting)

SUBSECTION(LAN game )
PARAGRAPH([
The fastest computer in your network should
act as the server. There, go to the network menu and hit the "LAN Game"-
menu item. After a second, PROGTITLE should tell you that there are no servers
currently available, but offer a "Host Game" item. Press Return on it. In
the following menu, you can select a name for your server and the game options.
The game options here are completely independent from those in single
player mode.
After everything is to your liking, you can hit the "Host Network Game"
menu item and the game will start on the server and
run just as in single player mode.
])

PARAGRAPH([
The other computers will be the clients. On them, you activate the "LAN Game"
menu item, too. This time however, there should be the server you just started
visible in the server browser. Just hit Return on it to connect.
])

SUBSECTION(Internet game)

PARAGRAPH([
An internet game works the same way; you just have to choose "Internet Game" instead of "LAN Game"
in the menu. Note that the number of users currently online on each server is displayed
by the server browser, too.
])

PARAGRAPH([
While in the server browser, you use "cursor left/right" to change the sorting
key, +/- to give individual servers a bias to the score (and thus their place
with on the list if you sort by score which is the default). Add servers to
your LINK([network.html#bookmarks],[bookmarks]) with "b" and refresh all servers
with "r".
])

PARAGRAPH([Internet server browsing would not be possible without master servers. We currently
use four masters,
one in Texas run by Lucifer, one in Virginia run by antix of 
ELINK(distortgaming.com,DistortGaming)
(Who will gladly host your game server, Armagetronad or other. Shameless plug end.)
one run on Z-Man's old, trusty laptop-gone-router in Germany, and another one in Germany run
by iF.
DNS service for swapping out masters without you having to update your configuration
is provided by Tank.])

SUBSUBSECTION(Current status)

PARAGRAPH([
Current counts show about 40 active servers.
If you're not picky about your fellow players, there should be
someone to battle against on one of those. The problems start as soon as you
develop a preference for certain server settings, because there are quite a
number of flavors around.
])

ifelse(,,,
[
PARAGRAPH([
At the time of this writing, there are about ten dedicated servers available, so you should
not need to start your own. Most of them will let you fight against a bunch of AIs
while you wait for other players to join in.
])

PARAGRAPH([
Finding an opponent to play against should not be a problem these times; nevertheless,
there still is the ELINK(WEBBASE/meeting.html, meeting
page). Check it out, be online when the countdown reaches zero and
you will certainly find another player online. I'm usually playing at these times and
specially like to be on a server that allows cockpit view only.
])
])

SUBSECTION(Bookmarks,bookmarks)

PARAGRAPH([
You can access and edit your server bookmarks in the "Server Bookmarks"
submenu of the "Network Game" menu. You can add bookmarks from the server
browser or manually add bookmarks via the "Edit Bookmarks" menu.
])

ifelse(,,,[
SUBSECTION(Fallback)

PARAGRAPH([
If the server browser does not work for some reason (i.e. no master server
available), you can still connect to a server if you know its network address or
hostname and the port it runs on. The "Custom Connect" menu item in the network
game menu serves that purpose.])
])

SUBSECTION(Version Control)

PARAGRAPH([
Server and client do not need to be of the same version. With default settings, the current
server is compatible with clients down to 0.2.0, and the current client with servers down to 0.2.0.
Obviously, some new settings were introduced since then; if you change them away from the default,
three things can happen when an old client connects:])
<list>
<li>Variant 0: Nothing. The old client is allowed to connect, and the setting stays like it was.
It's the player's problem if something goes wrong.</li>
<li>Variant 1: The offending setting automatically reverts to its default value.</li>
<li>Variant 2: The old client is not allowed to connect. The player is already informed of that
in the server browser.</li>
</list>

PARAGRAPH([
You will be informed on the console when you change one of the affected settings which variant
will be used for it, and clients up to which version are affected.])

PARAGRAPH([
Settings are classified into five groups: Breaking, Bumpy, Annoying, Cheating and Visual. Settings of
the "Breaking" group absolutely destroy the game for old clients when they are not on their default
values; setting a custom map file is the perfect example. The "Bumpy" group allows basic play
for old clients, but it's not likely to be a pleasant experience. Examples would be <code>CYCLE_DELAY</code>
and <code>CYCLE_TURN_SPEED_FACTOR</code>. The "Annoying" group allows more or less pleasant gameplay 
with only little disturbances that feel a bit like small bugs. The 
<code>CYCLE_RUBBER_MINDISTANCE</code> setting
family belongs to this group. When a client does not honor settings of the "Cheating" group, 
this is supposed to be considered cheating. An example is the <code>DOUBLEBIND_TIME</code>
 setting, and the
<code>CAMERA_FORBID_</code> settings would fall into that category, 
too (but all clients know about them). Lastly,
"Visual" settings only affect displayed information and have no influence on gameplay.
And in fact, there is a sixth group where the behavior is hardcoded to either variant 1 or 0.
])

SUBSUBSECTION(Customizing Version Control)

PARAGRAPH([
Which of the three behavior variants is used for settings of the five categories is 
determined by the <code>SETTING_LEGACY_BEHAVIOR_</code> settings in FILE(settings.cfg).
The default settings block old clients on Breaking, Bumpy and Cheating settings
(Variant 2), and ignore possible conflicts for Annoying and Visual settings (Variant 0). 
To override the group defaults, you can define exceptions for single settings; if you 
want to revert the setting <code>FOO</code> to its default when an old client connects,
then you set <code>FOO_OVERRIDE</code> to 1.
])

PARAGRAPH([
You can also lock out old clients independently of incompatible settings;
use the variable <CODE>BACKWARD_COMPATIBILITY</CODE> for that. Setting it to 0 will only
allow the most current clients to connect. You can also disable some new features to
be activated by setting <CODE>NEW_FEATURE_DELAY</CODE> to some positive value.
If both configuration variables are set to the same value, no feature will be enabled
or disabled just because an old client connects/disconnects. The numbers these two settings
refer to are raw network protocol versions; LINK(versions.html,see here) for a table
comparing them with release versions.
])

SECTION(Interpersonals)
Please behave like a human being on the game grid. Don't use offensive language
when asked, sometimes there are children online. Help newbies into the game. Follow
the rules of the server administrator or change the server you play on.

SUBSECTION(Chat)
PARAGRAPH([
Send your fellow players messages by hitting the chat key configurable in the "Player Setup" menu
(defaults to "s"). Usually, the message will go out to all players. Servers from 0.2.8.0 on support
IRC style extensions: By starting your message with "/msg <playername>", you can send selected 
players personal messages. With "/me", you can tell the others what you do; "/me falls asleep"
will print "Walruss walls asleep" if you're Walruss.
])

PARAGRAPH([
If another player annoys you, the silencing menu comes in handy: it can be reached by
pressing ESC, then selecting "Player Police/Silence Menu". If you're annoyed by chat in
general, edit FILE(settings.cfg) and activate SILENCE_ALL. This has the effect that new
players get silenced by default and have to be unsilenced if you want to hear them.
])

SUBSUBSECTION(Instant Chat)
PARAGRAPH([
By default, the game lets you say some predefined things when you press F1 to F12 or 1 to
Backspace. Beware, the default settings for those can get you in trouble.
Change them in "Player Setup/Player 1 Settings/Instant Chat Setup",
and change the keybindings used in the player input configuration.
If you are already in chat mode and hit one of the instant chat keys not associated
with a printable character, like the F keys, the corresponding instant chat
will be inserted where you type. If an instant chat string ends in a backslash "\",
pressing the corresponding key will not send the string immediately, but will let you
append to it first.])

SUBSECTION(Ingame Admin)
The server administrator can allow selected players to control the server settings through chat
commands; the setting SI(ADMIN_PASS) has to be modified to a nondefault value. Whoever knows
this value can log in by saying "/login &lt;password&gt;". After that, it's possible to issue regular
console commands by saying "/admin &lt;command&gt;" or to log out again with "/logout".
<br>
<strong>WARNING: currently, chat messages and therefore the password are sent unencrypted. This
means that everyone with physical access to the network flow, which is, among others, anyone
on the server's or the player's LAN, can steal the password.</strong>

SUBSECTION(Kicking and Banning)
PARAGRAPH([
You can kick players with the SI(KICK user) command; it accepts either the network user ID or
the user name (currently the nickname, filtered so you can actually type it) 
of the player to kick; you can get a list of both with the command SI(PLAYERS). You can
ban them from the server with SI(BAN user) or SI(BAN user time); the time duration is given
in minutes and defaults to 60 minutes if not present. The SI(user) argument of SI(BAN) is 
interpreted exactly as with SI(KICK).
])
PARAGRAPH([
You can get a list of currently banned IP addresses with SI(BAN_LIST). To unban one of these,
use SI(UNBAN_IP ip). To ban an IP address explicitly, use SI(BAN_IP ip). Bans are persistent
when you shut down and restart the server; the data is stored in FILE(var/bans.txt) as
IP/duration in seconds pairs.
])
SUBSUBSECTION([Autobanning])
PARAGRAPH([
When a user gets kicked often (by you or by kick votes), he will be banned automatically. It is
measured how often a user gets kicked per hour (KPH); this value is not persistent across
runs of the server. KPH can't get larger than SI(NETWORK_AUTOBAN_MAX_KPH). When a user
gets kicked, the duration of the automatic ban in minutes is determined as 
(KPH - SI(NETWORK_AUTOBAN_OFFSET)) * SI(NETWORK_AUTOBAN_FACTOR); the user is not banned if this value
is negative. You can disable automatic banning by setting SI(NETWORK_AUTOBAN_OFFSET) larger than
SI(NETWORK_AUTOBAN_MAX_KPH).
])


SUBSECTION(Voting)
Sometimes, votes on important topics will be cast by players. If a vote is pending, a menu
will automatically pop up between rounds and remind you to vote. You can accept or reject
a suggestion, or explicitly say you don't care about it.
<br>
The only thing possible to vote on are player kicks. To start a kick vote, press ESC and enter
the "Player Police/Kick Menu" menu and select the player you want kicked. Prepare to get kicked
yourself if you abuse the system.

SUBSECTION([Synchronization Settings])
PARAGRAPH([
The two CI(CYCLE_SYNC_INTERVAL_) settings determine the interval at which sync commands are sent
for cycles. CI(CYCLE_SYNC_INTERVAL_SELF) determines the time between syncs to the owner of
a cycle itself, and CI(CYCLE_SYNC_INTERVAL_ENEMY) determines the interval for everyone else.
])
 
New clients since 0.2.7.1 send the time of turn commands to the server. 
This makes it possible to avoid grinding lag sliding
(you move towards a wall, grind it shortly and turn away again, and you'll slide)
by letting the cycle on the server turn not before the time sent by the client.
At the low speeds before the grind, the positional command interpretion 
is inaccurate and will usually turn the cycle too early. 
 Now, old clients don't send the command time, so this code can't work. The lag sliding is a clear disadvantage, but the earlier turn is an advantage in some situations because it makes you cover more ground, so both the new and the old players have plenty of reason to complain if they are not treated equally. Therefore, when CYCLE_FAIR_ANTILAG is set to 1 and old clients are present, this code is deactivated. 
 
 When a cycle turns in free space, the server will try to follow the client's request by matching the turn position as closely as possible. Sometimes however there are large desyncs and clients sent silly turns halfway across the grid from their current position. So, for clients that send the command time, the server will execute turns only in a time window around that command time. The width of that window is determined by CYCLE_TIME_TOLERANCE. 
 
 I observed that old clients ( 0.2.7.0 and earlier ) would be more likely to pass through walls when they received a sync from the server shortly before. So, if you set CYCLE_AVOID_OLDCLIENT_BAD_SYNC to 1, the server will not send those syncs. Whether this helps or makes matters worse by not sending enough syncs is unknown, that's why it is a setting. 

SECTION(Technical details)

PARAGRAPH([
You are not limited to one player per computer;
on each of them you can play with up to four people. In the
precompiled version, there is a limit of 16 clients.
You can set the configuration variable <CODE>MAX_CLIENTS</CODE> to limit it further.
If you compile your PROGTITLE server yourself, 
you can increase the limit if you configure it with
<pre>
CXXFLAGS="-DMAXCLIENTS=X" ./configure
</pre>
where X is the number of clients you want to support.
])

PARAGRAPH([
If you are behind a masquerading firewall (such as a DSL router or the Microsoft connection sharing),
you cannot act as a server; your computer is then unreachable from the outside unless
you manage to forward port 4534 UDP connections from the firewall to your server.
Most software solutions and some standalone DSL routers offer this option, so you may be lucky.
])

PARAGRAPH([
The client may be behind a firewall as long as it allows outgoing UDP connections
on the PROGTITLE port.
])

SECTION(The dedicated server,dedicated)

PARAGRAPH([
Following the model of Quake 1-3, there is a special binary version of
the game available for download (or compile it yourself giving the
option OPTION(--disable-glout) to configure) that has all input/output
features disabled. If you start it, it will read the normal configuration files and
set up a network game according to the settings in
the game menu (Number of AI players, game mode and finish mode).
A dedicated server takes input from the keyboard and interprets
it just the way it does with the 
<a href="config.html#files">configuration files</a>.
Additionally to the usual configuration files, the dedicated server
will read the file FILE(everytime.cfg) from the FILE(var) directory
 before each round; it may be comfortable to place quickly changing 
settings there. You can join the game on the dedicated server just the way
described <a href=#np>above</a>.
])

PARAGRAPH([
The advantages of this solution are:
<ul>
<li>there is no longer a "privileged" player; all players are clients
to the dedicated server.</li>

<li>
the dedicated server can use it's full computing power just to
manage the game; it does not have to bother about input or 3d rendering.
So, the game is a bit smoother for all players.
</li>

<li>
the dedicated server should run on any 32 Bit Windows and about any UNIX; 
it does not need any special library installed (OpenGL, SDL, ...) , so 
it is easy to set up public internet servers.
</li>

<li>
The dedicated server uses less CPU power than a client because
it does not have to care about graphics and sound. It can be run on an older PC.
</li>
</ul>
])

SUBSECTION(Controlling network input)

PARAGRAPH([Often, it is
desireable to not only specify the port PROGTITLE listens on, but also the IP address.
Examples [include] LAN servers that run on a machine with connection to the internet or
servers on a server farm where each host is shared between many users and has a multitude
of IP addresses. For this purpose, the SI(SERVER_IP) setting has been introduced. Documentation
with example usage is provided in FILE(settings_dedicated.cfg).
])

SECTION(Ping Charity)

PARAGRAPH([
Ping charity can be configured along with your network bandwidth and other settings
in the "Network Setup" submenu in the "Network Game" menu.
])

PARAGRAPH([
It is the configurable part of the "equal ping" technology. In short,
if you have low ping and your opponent has high ping (ping: the time
it takes a message to travel from your computer to the server and
back, usually measured in milliseconds), you can take over some of his 
ping to make the situation more equal.
So, if you have ping 60, your 
opponent has ping 160 and you set the ping charity to at least 50
(more does not change the situation), you  will take over 50 ms of 
his ping, giving you both ping 110. If you set your ping charity to
20,  you will end up with ping 80, your opponent with ping 140. Of
course,  you may be greedy and set ping charity to zero, but
I suggest leaving it at the default value 100.
])

PARAGRAPH([
How does that "equal ping" thing work? It is not that complicated, but
for now, I rather keep the secret buried in the source code (too lazy
to explain it right now...).
])


SECTION(Spectator Mode)

PARAGRAPH([
In the player menu, there is the "Spectator mode" toggle; If you just
want to watch an internet game, connect to the server with spectator
mode enabled. Note that you will be almost completely ignored in
spectator mode: the other players won't know you are there at all,
you can't chat and the dedicated server will not bother to start
a game if only spectators are online (all you are going to get is
a black screen). Only the server administrator will get a message 
that a client connected.
])
PARAGRAPH([
Please note that this feature is unsupported; future servers may not allow
spectating in this way.
])

SECTION(Scores)

PARAGRAPH([
In a multiplayer game, every crucial action makes you gain or lose
points; after
<ul>
<li>a fixed number of rounds has been played</li>
<li>a fixed time elapsed or</li>
<li>a fixed score is reached</li>
</ul>

the team with the most points wins the match. If two or more
teams share the first place, the fight continues until there is
a unique winner. 
<br>
You can configure the score/time/rounds-limits in the file 
FILE(settings.cfg); scores and winners are logged in the file 
FILE(scorelog.txt) on the server.
])

PARAGRAPH([
If you are the only person on a dedicated server, a special
single player game is started (its parameters are determined in
the LINK([config.html#sphh],[SP_* variables])
 in FILE(settings.txt) on the server) to keep
you busy until someone else connects; the highscores in
this mode may be published by the server administrator.
<br>
The highest scores collected in a single player game are collected
in the file FILE(highscores.txt), the people with the most won multiplayer
rounds/matches are stored in FILE(won_rounds.txt) and FILE(won_matches.txt).
A ladder is stored in FILE(ladder.txt).
<br>
Note: these statistics have all not been adapted to team play and most of them
will store very odd results.
])

<a name=bb>SECTION(BIG BROTHER)</a>

PARAGRAPH([
As in any software downloaded for free, you can't be completely
sure whether PROGTITLE has secret functions that, for example,
spy on your system internals, exploit known Windows bugs to
get to your ISP's password, etc... and send this information
to the author. Of course, PROGTITLE does not do such a thing,
and you can check that in the source code.
<br>
But PROGTITLE DOES send some information out: If you connect
to the master server for the first time, PROGTITLE will send
<br>
<ul>
<li>Your Operating system and PROGTITLE/SDL version</li>
<li>the information the OpenGL system offers, telling me which
graphic card you use</li>
<li>if you are running PROGTITLE fullscreen or windowed</li>
</ul>

and nothing else, especially no personal information. I hope
you understand that there is a good reason to collect this information.
If you don't want to reveal these facts, simply edit
your PROGNAME configuration file FILE(user.cfg)
and change
<br>
<CODE>BIG_BROTHER 1</CODE>
<br>
to
<br>
<CODE>BIG_BROTHER 0</CODE>
<br>
])

SECTION(Internals)
PARAGRAPH([
If you are interested in network programming yourself, you may want to
read the <a href=net/index.html>network subsystem documentation</a>.
])

include(sig.m4)
include(navbar.html.m4)
</body>
</html>

