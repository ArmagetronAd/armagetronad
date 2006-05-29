include(head.html.m4)
define(SECTION,index)
include(navbar.html.m4)

TITLE(Armagetron: network code documentation)
<p align=justify><strong>
Warning: These document are basically obsolete. The demo programs don't work
currently and the docs for Layer 1 have not been updated for the recent refactoring.
However, Layer 2 and 3 still are mainly intact and give the basic ideas about
the system.
</strong></p>
<p align=justify>
This document is not designed to explain every detail of the network
subsystem; it is intended as a sort of user's manual for people who want to
integrate Armagetron's networking into other GPL'd games and don't know
much more about networking than the average Quake fan. You will not read 
anything about the "equal ping" technology here; I didn't know that before
I started writing this either :-) "equal ping" is not part of any of the
three network layers, it's in the way the game timer is handled and how
client control messages are interpreted by the server.
</p>

SECTION(General structure)

<p align=justify>
Just like most other network games, Armagetron uses a client/server
architecture. The server has full control over the game, to minimise the 
possibility of cheating: the clients send only their players' input to the 
server (and of course, don't wait for the server's response to interpret it),
the server executes the commands and sends all clients the updated game 
information.
</p>

<p align=justify>
As you probably expected, Armagetron's network code is organised in layers,
where the lower layers deal with the details of network communication and the 
upper layers use the functionality provided by the preceeding layer to 
implement more complex tasks. In detail:
</p>

<ul>
ITEM(<a href="lower.html">Layer 1 (Network protocol abstraction):</a>,
[This provides a network interface more or less independent of the system 
and network protocol. It was taken from ID Software's Quake, and supports
so far only UDP/IP on Unix and Windows. (With IPX loosing popularity, there
are not many other network protocols to support left...)])

ITEM(<a href="middle.html">Layer 2 (Network messages):</a>,
[Here, we implement a protocol similar to TCP (TCP guarantees that every
packet is received, and that the packets are received in the right order.
The latter property is not desired in a network game, as it causes heavy lag
every time a packet is lost and has to be sent again): it can send messages
to other computers in the game that are guaranteed to arrive. Additionally,
logging in and out of the server and bandwidth limitation is handled here.])

ITEM(<a href="upper.html">Layer 3 (Network objects):</a>,
[Now we get to the real C++ part: A base class for network aware objects (NAO)
is defined. Once created on one of the computers in the game, a NAO will 
automatically create copies of itself on the other computers.
If requested, a NAO will send update information to it's copies; the copies
may send player control messages to the original. (Cheating countermeasure:
normally, the original lives on the server and the copies on the clients.) 
If a NAO is destroyed, all copies will be destroyed, too.]
)
</ul>

<p align=justify>
The game itself uses the login/logout functions of layer two, and of course
a lot of layer three. The layers two and three are a little interconnected:
some stuff that should be happening in layer two already is postponed to
layer three (i.e. cleaning up after a client quit from the server), so it's
not possible to compile a program just with layers one and two.
Sorry about that...
</p>

SUBSECTION(Common data types)
<p align=justify>
<CODE>string</CODE> (declared in <CODE>smartstring.h</CODE>): this 
is a better substitute for the normal <CODE>char *</CODE> with several 
enhancements (you can simply append characters to it...).
</p>
<p align=justify>
<CODE>REAL</CODE> (declared in <CODE>stuff.h</CODE>): just a basic 
<code>float</code>.
</p>

SUBSECTION(Common global objects)
<p align=justify>
<CODE>con</CODE> (declared in <CODE>console.h</CODE>):
 this is the console and used just like <CODE>cout</CODE>
or any other stream. Messages written to <CODE>con</CODE>
are displayed on the screen or written to standard output.
</p>

<p align=center><!--#spaceportsbanner--></p>

include(sig.m4)
include(navbar.html.m4)

</body>
</html>

