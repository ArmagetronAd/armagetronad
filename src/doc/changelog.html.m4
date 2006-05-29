include(head.html.m4)
define(SECT,changelog)
include(navbar.html.m4)

TITLE(Changelog)

This page is kept for historical reasons only. See the file ChangeLog(.txt) or
HISTORY(.txt) for up to date information.

define(NEWSENTRY,<tr><td valign=top><strong>$1</strong></td><td>$2</td></tr>)

PARAGRAPH([
The name in () is the name of the person who suggested the changes/reported
the bug.
])
<table>
NEWSENTRY([2002/01/19],[Added moviepack title screen])

NEWSENTRY([2002/01/18],[Made speed and arena size configurable in the menus])

NEWSENTRY([2002/01/04],[Master server is now running])

NEWSENTRY([2001/12/12],[--version/-v and --help/h command line option for Linux (Adam Olsen)
])

NEWSENTRY([2001/12/14],[installation script and "make install" added])

NEWSENTRY([2001/11/20],[configurable network port (Justin Pinder)])

NEWSENTRY([2001/11/05],[allow darker bike colors (Danny Cohen)])

NEWSENTRY([2001/10/19],[Fixed grid simplification memory leak])

NEWSENTRY([2001/10/18],[Finished AI workover])

NEWSENTRY([2001/8/22],[Added custom camera mode and made camera positions
configurable])

NEWSENTRY([2001/8/02],[Added password saving])

NEWSENTRY([2001/8/01],[Completed logo display, fixed two network crashes])

NEWSENTRY([2001/7/31],[Better cycle color management: all color selections are now legal, but are changed on client side if a it gets too close to the floor color (which may vary from client to client)])

NEWSENTRY([2001/7/30],[Game is completely localizable and already translated to German])

NEWSENTRY([2001/7/17],[Made all console command help messages localizable])

NEWSENTRY([2001/7/12],[Reorganized lenght units: Walls are now one unit high.])

NEWSENTRY([2001/7/05],[Password querries for Krawall.de implemented])

NEWSENTRY([2001/6/30],[Master server, LAN polling and server browser finished])

NEWSENTRY([2001/6/23],[Network changes: Acks for messages not requesting an Ack are not sent any more; reorganized network message IDs])

NEWSENTRY([2001/6/22],[Begun work on Version 0.2.])

NEWSENTRY([2001/6/21],[Fixed a disappearing wall bug and avoided some cases
 of grid degeneration.])

NEWSENTRY([2001/6/20],[A gazillion bugfixes; among them the "Has no face in
direction..."-Bug and a parameter transfer bug.])

NEWSENTRY([2001/5/28],[Fixed bug that caused player kicks when ack packets got lost.])

NEWSENTRY([2001/5/23],[AIs now no longer attack themselves when they find noone
else to attack. They now find a target in network play.])

NEWSENTRY([2001/5/18],[Completed dedicated server for windows.<br>
Fixed some network bugs.<br>
Added buffer length option in sound settings.])

NEWSENTRY([2001/5/17],[Worked around ATI and S3 texture bugs. <br> Changing screen mode during the game now no longer makes the cycles invisible.])

NEWSENTRY([2001/5/16],[Inproved network performance: Ack packets are no longer Ack'd (makes no sense at all), NetObject IDs are requested in batches.])

NEWSENTRY([2001/5/10],[Finally corrected the "AI runs through outer wall"-bug.])

NEWSENTRY([2000/11/20],[Fixed minor bugs: the camera was reacting on walls
of already dead players, the menuitems in the player input menu were in a
bad order and the first game you watch after connecting to a server was
incredibly jerky.])

NEWSENTRY([2000/11/16],[Finished the first cleanup phase, containing really
a lot of bugfixes.])

NEWSENTRY([2000/07/30],[Fixed some nameless small bugs and crashes.])

NEWSENTRY([2000/07/28],[Improved behaviour of the glance keys in external camera modes (Robert Hagenstr&ouml;m)])

NEWSENTRY([2000/07/27],[Fixed two small bugs: the champion sometimes was
declared at the beginning of an unneccesary additional round (aLe&gt;&lt;),
 and a single player highscore hunt on a dedicated server started in round 2])

NEWSENTRY([2000/07/20],[The grid now is able to grow (in case a cycle makes it through the outer walll), so the rare ".. left grid" error should be fixed.<br>
Fixed the Voodoo3 mouse cursor bug (thanks to Andreas).])

NEWSENTRY([2000/07/15],[Improved control over own bike in network mode. Bad side effect: you can't connect to a server running an older version (the combination old client-new server works, though)<br>
Some internal changes that will allow shooting holes in walls, finite length
walls and continuous play])

NEWSENTRY([2000/06/04],[fixed some hangups (Seven3); game menu now pauses
the game if not in a network game (Seven3)])

NEWSENTRY([2000/05/30],[really fixed the latest exit crash, the camera
          hickups, a score bug, (hopefully) the VooDoo problems. Added
	  <a href=index.html#cl>command line switches</a>.])

NEWSENTRY([2000/05/29],[Released version 0.1.3; 
	  the <a href=index.html#fb>mailing lists</a> are now ready for you to subscribe.])

NEWSENTRY([2000/05/27],
          [Added Quake style <a href="config.html#console">console</a>
           input to the game and
	   the dedicated server; still not very user friendly, but
           you can live with it.<br>
	   Added Window manager and shortcut icon (Ian Novack)])

NEWSENTRY([2000/05/26],
          [another alt-tab lockup fix....<br>
	  Changed the documentation to HTML])

NEWSENTRY([2000/05/25],
          [Fixed alt-tab lockup and exit crash (hopefilly)])

NEWSENTRY([2000/05/24],
          [Prepared and released MSVC++ version])

NEWSENTRY([2000/05/23],
          [Discovered that my development system  Windows was 
	   malconfigured. It was a small miracle that Windows AT 
	   for SDL 1.1 ran at all. Well, it is fixed now.])

NEWSENTRY([2000/05/22],
          [fixed performance problems in network mode (RuRu Team)<br>
           added crude custom screen resolution  (Thomas Berry)<br>
	   fixed some login related bugs (perhaps) (RuRu Team)<br>
           current player list on my web site (Andre Messier)<br>
	   moved the moviepack and text output menuitems to the misc menu<br>
	   added configurable start speed and increased it to speed
            up confrontation in multiplayer mode])

NEWSENTRY([2000/05/21],[added spectator mode and express game restart option
	     (Martin Rauscher)])

NEWSENTRY([2000/05/20],[fixed some strange camera bugs (hopefully)
	     and one network crash bug<br>
	     added brake button (of course, disabled on the
	     internet server for compatibility)])

NEWSENTRY([2000/05/19],[removing players now works properly in a network game
             <br>
	     added ladder
	     fixed some injustice in ladder])

NEWSENTRY([2000/05/18],[added highscore lists and message of the day<br>
	     settings are now loaded from "autoexec.cfg", too<br>
	     fixed the sparks<br>
	     added "Rubber" niceness setting to improve internet play])

NEWSENTRY([2000/05/17],[finished rewriting the object management<br>
	     removed the "ghost bike" bug and many crash bugs.<br>
	     added the "Misc" submenu in the main menu with
	     <ul>
	     <li>menu selection wrap toggle (folbe)</li>
             <li>global keys for
	     <ul>
              <li>scores (Odin77)</li>
              <li>chat console scrollback (forgot on whose suggestion...)</li>
             </ul></li></ul>
             scores (in multiplayer mode) are logged in the file 
             "scorelog.txt"<br>
             Added "Big Brother" hardware reporting system, results
             are stored in "big_brother"])

NEWSENTRY([2000/05/13],[started rewriting the object management to increase
             network stability])

NEWSENTRY([2000/05/11],[added player scores and score table key (Odin77 and
             Ruhrpott-Ruler organizers)])

NEWSENTRY([2000/05/11],[Almost full support for moviepack 
                        (title screen missing) and movie sounds<br>
               	     switched to SDL 1.1.2])

NEWSENTRY([2000/05/10],[improved network synchronisation])

NEWSENTRY([2000/05/09],[Source code is now MSVC++ compatible<br>
  	     texture modes may be different for Floor, Walls, Cycles
          	     and font now<br>
             added default fov and glance back (Odin77)])


NEWSENTRY([2000/05/08],[AI players now avoid having the same color as you])

NEWSENTRY([2000/05/06],[Added instant chat macros, spam protection and
	     glance keys])

NEWSENTRY([2000/05/05],[Added option to disable the automatic switching to the
                internal camera (MaXiM)<br>
	     Moved camera setup in a separate submenu<br>
	     added camera preselection<br>
	     added server-side configuration that is transferred<br>
	       to the clients (cycle speed, forbid some cameras..)<br>
	     it is no longer possible for one player to have two viewports])

NEWSENTRY([2000/05/04],
          [Added first moviepack support
          (direct .ASE file loader)<br>
          added player menu to ingame menu])

NEWSENTRY([2000/05/03],
            [PROGTITLE detects now when you switch from harware to
             software rendering<br> At first start, a default keyboard
             config is loaded<br> Improved timing on login and cylce
             controls in a network game.])

NEWSENTRY([2000/05/02],[Bugfixes: fixed laggometer<br>
             added "Boss Key": shift-esc quits from everywhere])

NEWSENTRY([2000/04/28],
          [Bugfixes: added in-game menu, some minor display changes,
           the chatting AI now really works, added binary for SDL 1.1.])

NEWSENTRY([2000/04/25],[After the long break caused by me moving to cologne,
             I finally removed that chatting timer bug. Cleaned up
             and documented the network system a bit. Now, while
             chatting, a simple AI takes over for you, and a yellow
	     thing circles above your head to show the other players
             you are not fully able to defend yourself.])

NEWSENTRY([2000/03/29],[Added Sound, fixed a bug that made it impossible to
             connect with more than one client to a server; 
             released version 0.1.])

NEWSENTRY([2000/03/25],[You now can acually see what other people are saying;
	     fixed a bug in the console output. Last release before 0.1.
	     Really fixed cursor bug :)])

NEWSENTRY([2000/03/23],[Optimized the network code for high ping play; simulated ping 500... Removed cursor bug.])

NEWSENTRY([2000/03/22],[Added chatting and "equal ping" technology; network protocol will not change any more until after version 0.1])

NEWSENTRY([2000/03/21],[  Major change in network protocol; it is now fully
	     portable. (I had to set up a server on an PowerPC AIX system...)
	     tested internet play a bit (the ping was only 50ms; no
	     big problems seen.)])

NEWSENTRY([2000/03/20],[Various internal network mode improvements and bugfixes])

NEWSENTRY([2000/03/18],[Split the web page in five sub-pages
	     created a dedicated server for internet play (still not
	     properly tested), plan to install it at aixterm on Monday.
	     The network code has now a transfer rate limit.])

NEWSENTRY([2000/03/17],[Removed two more network bugs, switched to SDL 1.1.1 .
	     Put some order in the "display settings" menu.
	     The plasma sky is in again. There probably are some
	     bugs in the new code; if you find a combination of
	     sky/rim/ztrick settings that does not work, mail me about it!])

NEWSENTRY([2000/03/08],[The windows timer was ugly. Smoothed it a bit.
	     Removed most network bugs, improved camera movement.])

NEWSENTRY([2000/03/07],[Finished Network mode! You now can watch other players
             when you have died and switch between them by pressing
	     left/right.])

NEWSENTRY([2000/03/04],[Switched from the SDL timer functions to the ones from
	     the Q1 source (better resolution). Removed a bug from the
	     Linux version that caused a lockup on the first start
	     of PROGTITLE.])

NEWSENTRY([2000/02/22],[Begun work on network code by stealing net_udp.c and
	     net_udp.h from ID's Q1 sources.])

NEWSENTRY([2000/02/21],[Unfortunately, yesterday's changes do not work in 
             Windows.
             Disabled them for the Windows version.])

NEWSENTRY([2000/02/20],[Input is now asyncronous to graphic 
             refresh to allow more precise
	     control on slow machines.])

NEWSENTRY([2000/02/15],[renamed it from "Walls" to "PROGTITLE".])

NEWSENTRY([2000/02/13],[improved visibility caltulation to support walls of 
             different height and viewers at different heights])

NEWSENTRY([2000/02/11],[Added "AI" players])

NEWSENTRY([2000/02/10],[Added external camera perspective])

NEWSENTRY([2000/02/09],[Fixed bugs: timer jumps, welcome screen, 3dfx windows
	     renderer default settings.
	     Added floor reflection, true floor grid rendering
	     using dual textures and detail settings for the two.])

NEWSENTRY([2000/02/08],[Number of players is now 2-4; took some
	     time trying to make it compatible with gcc-2.7.2.3.])

NEWSENTRY([2000/02/06],[Player input is now configurable and easy to extend.])

NEWSENTRY([2000/02/04],[added savable configuration, a rudimentary main menu
             and the possibility to change screen resolution.])

NEWSENTRY([2000/02/03],[added text output and menu system; 
              found nasty memory eater
             (Mesa only? Texture memory is never freed,
	      even after DeleteTextures(). Fix: use texture id as a
	      substitute...)])

NEWSENTRY([2000/02/02],[added explosions.])

NEWSENTRY([2000/02/01],[added a "wall rises out of the floor"-effect,
             changed wall texture to better fit it;
	     fixed two bugs (linux crash and windows texture problem)
	     added model loader and cycle model with turning wheels.])

NEWSENTRY([2000/01/28],[first test release. Two players on one computer.])
</table>

include(sig.m4)
include(navbar.html.m4)
</body>
</html>
