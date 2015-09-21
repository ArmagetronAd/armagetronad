include(head.html.m4)
define(SECT,faq)
include(navbar.html.m4)

TITLE(Frequently Asked Questions)

This list is poorly maintained and can't answer the current real FAQs because at
the time of this writing, they weren't asked yet :) Check out the
ELINK(wiki.armagetronad.net/index.php/FAQ,FAQ on the Wiki), too.

define(FAQQ,<li><table><tr><td valign=top>Q:</td><td><strong>$1</strong></td></tr>
           <tr><td  valign=top>A:</td><td>$2</td></tr></table></li>)

<ol>

<li>OS Specific Questions:</li>
<ol>

ifelse(DOCSTYLE,unix,,
[
<li>Windows:</li>
<ol>

ifelse(,,,
[
FAQQ([Ack! Why is the windows executable so big?],
[I think it is because it is statically linked against
some big libraries, but I am not sure. (Don't moan. You can
choose: static linkage =&gt; big executable or dynamic linkage
=&gt; I have to include many big DLL-files.)
Compiling with MSVC++ helps, but I am not allowed to distribute
the binaries I produce with that...])
])

FAQQ([<a name=3d_windows>How</a> can I enable 3D acceleration?],
[
Go to the web page of your graphic card vendor and look for help there. Most of the time,
a driver update should do the trick. You can check there if 3D acceleration is supported
for your version of Windows, too. For an automated setup for older video cards, 
try ELINK(www.glsetup.com,GLSetup).
])

</ol>
])

ifelse(DOCSTYLE,windows,,
[
<li>Linux:</li>
<ol>FAQQ([Immediately after PROGTITLE opens it's window, it crashes. What's wrong?],
[First, check Question 3.1 for the OS independent answer to that problem.
Special Linux case: In some rare cases ( old 3dfx Mesa implementation ), 
you need to be root to run 3d accelerated programs
Try running PROGTITLE as root; if that works, check out the HOWTOs and FAQs for your
graphic card on how to run 3d programs as non-root.
])

FAQQ([The textures and fonts don't seem to work.],
[Recheck your SDL_image installation; it probably can't load PNG images. 
The included show-program
has to be able to display the PNG images from the textures folder.])

FAQQ([<a name=3d_linux>How</a> can I enable 3D acceleration?],
[
You may have to download some stuff and modify your kernel module configuration and
XF86Config. That sounds scarier now than it is in reality.
For generic instructions, have a look at ELINK(www.xfree86.org/4.0/DRI7.html, the XFree DRI documentation). For NVIDIA cards,
get the latest drivers from ELINK(www.nvidia.com,NVIDIA) directly.
])

FAQQ([When trying to run PROGTITLE, I get the message 
"error while loading shared libraries: libXXX: cannot open shared object file: No such file or directory".
What's wrong?
],
[
You probably installed a binary version of PROGTITLE that is incompatible with your system.
Check if you have installed all the required LINK(install_linux.html#libs
,libraries).
One of the ELINK(armagetron.sf.net/download_linux.html#thirdparty
,[third party packages]) may fit better to your system. 
Or, if you want the latest version, try installing from source.<br>
If you installed PROGTITLE from a RPM without warnings, that RPM is broken; please
ELINK([armagetron.sf.net/contact.html],inform me) about it.
])

ifelse(,,,[
FAQQ([The sound has a very high latency. What can I do?],
[
PROGTITLE does not seem to work very well with some sound daemons. Try building it with
the OPTION(--disable-music) option to configure.
])
])

</ol>
])
</ol>

define(REFER_3D,[Check out the OS specific answer to "How can I enable 3D acceleration?" for
ifelse(DOCSTYLE,unix,,
[
LINK([faq.html#3d_windows
], Windows)]) dnl
ifelse(DOCSTYLE,web,or) dnl
ifelse(DOCSTYLE,windows,, dnl
[LINK([faq.html#3d_linux
], Linux)])]) dnl


define(USER_CFG,[your FILE(user.cfg) (found in the FILE(var)-subdirectory
ifelse(DOCSTYLE,unix,, of the PROGTITLE directory) 
ifelse(DOCSTYLE,web,in Windows resp.) 
ifelse(DOCSTYLE,windows,,of the directory ~/.PROGNAME)
ifelse(DOCSTYLE,web,in Unix)
 )
])

define(SEND_CFG,[send me (MAILMANGLE(z-man,users,sf.net))  your USER_CFG])

<li>graphics:</li>
<ol>
FAQQ([The graphics are ugly! There are almost no textures around, the menu is just writing on
black background. What can I do?],
[PROGTITLE detected that your OpenGL implementation does not support hardware acceleration.
It may be wrong on that; check it by increasing the detail level in "System Setup/Display Settings/Detail Settings".
If you tune up the texture modes and get an immediate performance dropdown, you are using
software rendering. REFER_3D. <br>If the rendering speed stays OK, you have hardware acceleration;
in that case, please SEND_CFG so I can correct the detection in future versions.
])

FAQQ([Rendering is VERY slow, I only get about 1 fps even in the menu. What's wrong?],
[It seems your OpenGL implementation does not support hardware acceleration, and PROGTITLE
did not detect it. REFER_3D. 
<br>
Please SEND_CFG so I can correct the detection in future versions. 
<br>In the
meantime, edit this FILE(user.cfg) and set TEXTURE_MODE_0/1/2 to -1 and ALPHA_BLEND to 0,
this will disable the features that cause the slowdown.
])
</ol>

<li>generic stuff:</li>
<ol>
FAQQ([<a name=startfaq>When</a> I start PROGTITLE, the screen turns black and the game
exits shortly afterwards. (Or similar right-after-start
problems) What can I do?],
[just try again about four times; PROGTITLE should notice that
the first attempts failed and try some other screen modes (with different
colour depths, initialisation methods and in fullscreen or windowed mode).
])

FAQQ([I can't join a game! When I start a local game, I only see AIs fighting, and
in a network game, I can only watch the others. I can't even chat! What's wrong?])
,
[
You are stuck in spectator mode.
It has been reported to get activated by itself on random occasions
(upgrading etc.). Go to "Player Setup"/"Player 1 Settings" and change "Spectator Mode" to "Off".
])

</ol>

<li>network play:</li>
<ol>

FAQQ([Why can't I play on my LAN? The server my friend started is invisible, and when
I do a custom connect, the connection fails.],
[You probably have an activated personal firewall running on your machine blocking
the network communication.
You have to set your firewall to allow outgoing UDP connections on port 4533 (for the
master server) and usually port 4534 for the servers. Note that some versions of MS Windows (XP,
I think) install a firewall by default that blocks all unknown connections, so you may not
be aware you have a firewall running at all.
])

FAQQ([Why can't I play on the net? When I try an Internet game,
I get an error saying the master server is not responding.],
[It may be that the master server is temporarily down. It is only running on my now stationary
laptop connected by DSL; that is not completely failsafe. The previous question may give some insights, too.
])

FAQQ([Sometimes, I see players (esp. me) passing through walls. Is that a bug?],
[Probably not; it is the normal effect of network latency (Laaaag!)
when your client does not yet have the latest information from the server. But
yes, if the effect is permanent (you really end up on the other side), it is a
bug you should report.])

FAQQ([Someone is cheating! I keep cutting off his way right before him, and
that kills me, not him!],
[Again, this is not cheating, but a network latency effect that happens
when you disable "Prediction" from the network game menu. Then, you see
the other players not at the place they are probably now, but where they
were a little while ago. So while you thought you were ahead of your opponent,
he was in reality a bit ahead of you. By trying to cross him, you ran directly
into his wall. Either turn on prediction or the "Lag-O-Meter" that shows
where your opponent may in fact be NOW. Learning to handle these effects is
one of the skills you need to kick your opponent's ass.])

<li>dedicated server:</li>
<ol>
FAQQ([I started a server, but nobody sees it on the master list. What's up?],
[
You need to set <code>TALK_TO_MASTER 1</code> in FILE(settings_dedicated.cfg). Our
default polity is not to make network connections if not explicitly enabled;
the connection to the master server is no exception. Plus, we like it when the
administrators of public servers read the documentation before they put up a
public server, and disabling the master server connection by default apparently
has this effect. You're not reading this FAQ just for fun, right?
])
FAQQ([I started a server with <code>TALK_TO_MASTER 1</code>, but can't see it on the master server list. What's up?],
[
That's normal for default settings if the server and your client are on the same LAN;
when the client polls your server, the server sends its response packets over the LAN,
but the client expects them from the internet. We found no failsafe way to avoid this
problem yet; you can correct it by setting <code>SERVER_IP &lt;server's public internet IP&gt;</code>
in FILE(settings_dedicated.cfg). Instead of the raw IP, you can enter a DNS name that will resolve
to your correct IP.
])
FAQQ([How do I get rid of AI players in multiplayer games?],
[
Use the settings
<pre>
AUTO_AIS 0
MIN_PLAYERS 0
NUM_AIS 0
</pre>
])
FAQQ([No, I mean ALL AIs. The ones that join human teams bug me, too.],
[
That would be
<pre>
TEAM_BALANCE_WITH_AIS 0
</pre>
])
FAQQ([And how do I get rid of teams? On an internet server with people joining and leaving all the time, they are confusing.],
[
That's
<pre>
TEAM_MAX_PLAYERS 1
</pre>
])
FAQQ([I don't like the way braking works. How can I change that?],
[
To disable braking, use
<pre>
CYCLE_BRAKE 0
</pre>
To turn the brake into a turbo, set it to a negative value.</br>
To reenable the brakes as they were before 0.2.5 (unlimited use), use
<pre>
CYCLE_BRAKE_DEPLETE 0.0
</pre>
])
FAQQ([How about restricting all clients to cockpit view?],
[
You are starting to bug me. Use
<pre>
CAMERA_FORBID_SMART 1    # forbid smart camera
CAMERA_FORBID_IN    0    # forbid internal camera
CAMERA_FORBID_FREE 1     # forbid free camera
CAMERA_FORBID_FOLLOW 1   # forbid fixed external camera
CAMERA_FORBID_CUSTOM 1   # forbid custom external camera
</pre>
I personally really like these settings, but most users seem to disagree there.
])
FAQQ([And what about...?],
[
Gaa! Enough is enough! This is a FAQ, not a ACQ (All conceivable questions ).
Read at settings.cfg and settings_dedicated.cfg and the 
LINK(config.html#files
,documentation). Yes, it is long. If you don't find your answer there,
look for help ELINK([forums.armagetronad.net/viewtopic.php?t=1360],[on the Forums]).
])

</ol>
</ol>
</ol>

include(sig.m4)
include(navbar.html.m4)
</body>
</html>
