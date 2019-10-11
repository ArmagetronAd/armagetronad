include(head.html.m4)
include(navbar.html.m4)

TITLE(Protocol Version History)

PARAGRAPH([
TABLE([
TROW2(STRONG(PROGTITLE Version),STRONG(Protocol Versions supported))
TROW2(0.2.0 - 0.2.4,0)
TROW2(0.2.5.x ,0 - 2)
TROW2(0.2.6 - 0.2.7.0 ,3 )
TROW2(0.2.7.1 ,4 )
TROW2(0.2.8.0_beta1,6 )
TROW2(0.2.8.0_beta2,7 )
TROW2(0.2.8.0_beta3,8 )
TROW2(0.2.8.0_beta4,9 )
TROW2(0.2.8.0_rcX,10 )
TROW2(0.2.8.0,11 )
TROW2(0.2.8.2,13 )
TROW2(0.2.8.3,16 )
TROW2(0.2.9 snapshots,17 )
TROW2(0.3.1,20 )
TROW2(0.3.1_pb,21 )
],cellpadding="5" border="1")
])

PARAGRAPH([
TABLE([
TROW2(STRONG(Protocol Version),STRONG(New Features))
TROW2(1,Protocol version control)
TROW2(2,Instant win zone and limited usability of brakes)
TROW2(3,Chat control( silencing and kick voting ) )
TROW2(4,[Fixed the rip bug, clients sent game time of cycle commands])
TROW2(5,[new cycle sync code: server sends more data to identify which client command was last executed. Shaped arenas and non-rectangular turns added.])
TROW2(6,[Client support for server formatted chat messages.])
TROW2(7,[Doublebind countermeasures. Proper cycle speed and acceleration handling and syncing.])
TROW2(8,[Server bugfix: turns that were accepted from the client, but not yet executed, are not claimed to be executed in sync messages.])
TROW2(10,[New server controlled voting items to add voting items that only require code on the server (code was broken and is no longer used).])
TROW2(11,[No actual change, just the bump to 0.2.8.])
TROW2(12,[Again, no actual change to be found. Z-Man wonders why.])
TROW2(13,[Spectators can now be visible and chat, clients no longer send redundant brake commands.])
TROW2(14,[Clientside lag compensation and server controlled ping charity, fullscreen messages.])
TROW2(15,[Player authentication, new fixed version of the server controlled vote items.])
TROW2(16,[Bugfix: clients no longer get confused if they have to move a cycle backwards during a sync from the server.])
TROW2(17,[])
TROW2(18-19,[Reserved for future usage in legacy versions])
TROW2(20,[Zones with more shapes than circles introduced in 0.3.1])
TROW2(21,[Network core switched to protobuf messages])
],cellpadding="5" border="1")
])

PARAGRAPH([
Note: Some protocol versions were only available in CVS/SVN/BZR, so not every protocol version corresponds to a PROGTITLE version. Likewise, not every version had a network protocol change.
])

include(sig.m4)
include(navbar.html.m4)
</body>
</html>

