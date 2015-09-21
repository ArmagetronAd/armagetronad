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
TROW2(0.2.8.0 - ,10 )
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
TROW2(10,[New server controlled voting items to add voting items that only require code on the server.])
],cellpadding="5" border="1")
])

PARAGRAPH([
Note: Some protocol versions were only available in CVS, so not every protocol version corresponds to a PROGTITLE version.
])

include(sig.m4)
include(navbar.html.m4)
</body>
</html>

