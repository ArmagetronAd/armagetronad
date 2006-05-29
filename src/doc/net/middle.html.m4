include(head.html.m4)
define(SECTION,middle)
include(navbar.html.m4)

TITLE(Layer two: network messages)
<p align=justify>
This layer is responsible for the basic communication in a client/server
framework (Files: <CODE>network.h</CODE> and <CODE>network.C</CODE>).
It allows the clients to connect to/disconnect from a server and 
provides a way to send messages in both directions; each server/client 
gets a unique user ID (zero for the server, 1...n for the clients).
The average round trip time of network packets (the ping) is estimated
and stored in <CODE>REAL avg_ping[[user ID]]</CODE> (unit: seconds).
A bandwidth limitation and message queue based on priorities is provided. 
For every message the
server/client receives, an acknowledgement is returned. If no acknowledgement
for a sent message is received within a reasonable time (about ping*1.1),
a message is considered lost and is sent again.
</p>

SECTION(Usage)
<p align=justify>
A sample program for layer two is 
<a href=../../src/network/l2_demo.cpp><CODE>l2_demo.cpp</CODE></a>
from the source directory. Compile it with <CODE>make
l2_demo</CODE>; the syntax is <CODE>l2_demo</CODE> to start it in 
server mode, just listening to clients, or 
<CODE>l2_demo servername</CODE> to start it in client mode 
connecting to the server
given by <CODE>servername</CODE>; it will send some simple messages from
the client to the server, who will display the results.
</p>

SUBSECTION(Bandwidth control)
<p align=justify>
Before starting up the network, set <CODE>max_in_rate</CODE>
and <CODE>max_out_rate</CODE> to the input/output bandwidth (in kB/s) 
of the used network interface. It's best to let the user decide that.
The network subsystem will take care that the actual rates stay below
these given numbers, avoiding massive problem with lost packets.
</p>

SUBSECTION(Communication setup)
<p align=justify>
Armagetron's network subsystem can be in three different states: 
standalone (no networking), server, or client (connected to a server). 
The current state can be read via <CODE>get_netstate()</CODE> (and is 
an enum type <CODE>netstate</CODE>, which can be one of 
<CODE>NET_STANDALONE, NET_SERVER</CODE> and <CODE>NET_CLIENT</CODE>);
<CODE>set_netstate(netstate state)</CODE> is used to set the current state.
The client state is most conveniently set by 
<CODE>void connect(const string &server)</CODE>, which will set the state and
establish a connection to the given server; check with 
<CODE>get_netstate()</CODE> whether the login was sucessfull. Logging out 
is simply done with <CODE>set_netstate(NET_STANDALONE)</CODE>. 
When switching between server and client state, one should visit 
the standalone state in between.
</p>

SUBSECTION(Network loop)
<p align=justify>
The network subsystem does not use threads; so, to receive network messages,
you have to call the function <CODE>receive()</CODE> every once in a while
to get the messages the other computers send. Do it at least once in the game
loop. <CODE>receive()</CODE> is responsible for sending away queued messages, 
too.
</p>

SUBSECTION(Messages)
<p align=justify>
Before writing a piece of code that sends a message to another computer,
you have to decide what the receiver should do with it. Layer two already
takes the responsibility to sort the incoming messages by type,
so there's no need to write one big <CODE>receive</CODE> function that analyses
and processes the messages. Instead, for every type of message (player moves,
player dies, player shoots,...) you want to have, you write an own small
receive function, accepting a reference to an object of the class 
<CODE>netmessage</CODE>, for example, if you want to handle a
"player dies"-message consisting of only one <CODE>short</CODE> containing
the ID of the player dying, you write
</p>
<pre>
void kill_handler(netmessage &m){
  short player_id;
  m &gt;&gt; player_id; // read the ID from the message

  // do some security checks; is the computer we got the message
  // from really allowed to kill the player? The user ID of the
  // message's sender can be read by m.net_id(). If he's cheating,
  // kick him by throwing a killhim-exception.

  .......

  // kill player player_id.

  .......
}
</pre>
<p align=justify>
Then, you need to define a class of messages responsible for killing players;
to do that, you create an object of the class <CODE>netdescriptor</CODE> with
your message handler's address and a nice name as arguments, i.e. by writing
</p>
<pre>
static netdescriptor kill_descriptor(&kill_handler,"Kill");
</pre>
<p align=justify>
directly after <CODE>kill_handler</CODE>. To send a message, you have to
create an object of class <CODE>netmessage</CODE>
with <CODE>new</CODE> and your descriptor as argument:
</p>
<pre>
netmessage *m=new netmessage(kill_descriptor);
</pre>
<p align=justify>
Then, you write the data into the message, in our example only
</p>
<pre>
short this_player_has_to_die=3;
(*m) &lt;&lt; this_player_has_to_die;
</pre>
<p align=justify>
And send the message to the computer with user ID <code>peer</code>
with the message's member function 
<CODE>send(int&nbsp;peer, REAL&nbsp;priority=0, bool&nbsp;ack=true)</CODE> 
or to all connected
computers (the server if the network subsystem is in client state, all
clients if it's the server state) via the member function 
<CODE>broadcast(bool&nbsp;ack=true)</CODE>, in our case most conveniently
</p>
<pre>
m->broadcast();
</pre>
<p align=justify>
Normally, the message is guaranteed
to arrive sooner or later (and is sent again if it gets lost). Not so important
messages (like position updates in a racing game) 
may be market by giving <CODE>ack=false</CODE> as an argument.
As usual, giving a lower  <CODE>priority</CODE> than 0 will make the
message more urgent, giving a higher <CODE>priority</CODE> will make it sent
later (<CODE>priority</CODE> is given in seconds; that is, a message with
priority one, already waiting for one second, is considered 
as important as a message with priority zero that just entered the queue).
DO NOT use <CODE>delete</CODE> to get rid of the message; it will delete
itself as soon as it no longer needed. Of course, feel free to encapsulate
all the steps above in a derived class of <CODE>netmessage</CODE>.
</p>
<p align=justify>
And that's about all there is. With the operators <CODE>&lt;&lt;</CODE> and
<CODE>&gt;&gt;</CODE>, 
you can write/read shorts, ints, REALs and strings. All data
is sent in a portable manner; you do not have to worry about endianness or
different float formats. If you want to send/receive messages with variable
length, you can use the member function <CODE>netmessage::end()</CODE>. It
tells you whether the message you are just reading out with <CODE>&gt;&gt;</CODE>
has been fully read. (Any more reads when <CODE>end()</CODE> returns 
<CODE>true</CODE> result in a network error, causing the connection to
be brought down.)
</p>

SUBSECTION(Some special functions)

CODEITEM(client_con,void client_con(const string &message,int client=-1);,
Lets client No. <CODE>client</CODE> display <CODE>message</CODE>
on his console. If <CODE>client</CODE> is left to <CODE>-1</CODE>[,]
all clients display the message.)

CODEITEM(client_center_message,
void client_center_message(const string &message,int client=-1);,
[The same as above, only the message is displayed in large letters at
the center of the screen.])

CODEITEM(sync,void sync(REAL timeout,bool sync_netobjects=false);,
Synchronises the clients with the server; waits until all
queued network packets are sent or <CODE>timeout</CODE> seconds pass.
Umm[,] already a part or layer three: if <CODE>sync_netobjects</CODE>
is set to <CODE>true</CODE>[, the network objects are synchronised, too.])


SECTION(Protocol details)
<p align=justify>
A network message is sent in the following structure (all data types are 
<CODE>unsigned short</CODE>s):
</p>

<table border>
<tr>
<td valign=top>Descriptor ID</td>
<td>The type of message (login, logout, acknowledgement, user input...).
Determines what message handler is called at the receiver.</td>
</tr>
<tr>
<td valign=top>Message ID</td>
<td>A locally unique identification number. Used for the acknowledgement and 
for discarding double messages.</td>
</tr>
<tr>
<td valign=top>Data length</td>
<td>The length of the following data in <CODE>short</CODE>s;
 used mainly to catch errors.</td>
</tr>
<tr>
<td valign=top>Data</td>
<td>The real message; everything that was written by <CODE>&lt;&lt;</CODE></td>
</tr>
</table>

<p align=justify>
Since every network protocol has a considerable amount of overhead per call 
of <a href=lower.html#write><CODE>ANET_Write</CODE></a> (56 bytes in the case
of UDP, I think...), multiple messages are transmitted with each low level 
write operation. The data send with each 
<a href=lower.html#write><CODE>ANET_Write</CODE></a> has the
following form:
</p>

<table border>
<tr><td>Message 1</td></tr>
<tr><td>.<br>.<br>.</td></tr>
<tr><td>Message n</td></tr>
<tr><td>Sender's user ID (as always, as a <CODE>unsigned short</CODE>)</td></tr>
</table>

<p align=justify>
The user ID is sent to simplify the server distinguishing the clients; with the
possibility of clients hiding behind masquerading firewalls and thus appearing
to send from changing ports, simply checking the sender's addresses may not 
be enough.
</p>

include(sig.m4)
include(navbar.html.m4)

</body>
</html>

