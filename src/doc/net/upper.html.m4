include(head.html.m4)
define(SECTION,upper)
include(navbar.html.m4)

TITLE(Layer three: network aware objects)
<p align=justify>
Layer three (files:<CODE>netobject.h</CODE> and <CODE>netobject.C</CODE>)
uses layer two to define a base class for classes of network
aware objects (NAOs). When a NAO is created on one of the computers in 
the network game, copies of it will be created on the other
computers. If the state of the original changes, it can send sync messages
to it's copies, making the same changes on them. Every NAO has an unique ID
number (shared with it's copies) and an owner (usually the computer which
created the original NAO, or the computer the player controlling the NAO
is using). If desired, a security system (against cheating) 
allows the original NAO only 
to reside on the server, and sync messages only to flow from the 
server to the clients. Only the NAO's owner is then allowed to send
control messages to the original NAO on the server, which will send back
sync messages reporting the changes made. Confused? Let's look at it
again:

SUBSECTION(NAO class with security disabled)
<p align=justify>
That's the easy part. Any computer can create/delete such an NAO and 
automatically owns it. Copies of the NAO will spawn/be deleted on all the other
computers. The owner is allowed to change the NAO (i.e. move it one meter
to the left) and issue sync commands;
then, sync messages will be sent to all other computers in the game
(if the NAO was created on a client, they will be directed through the server)
transferring the NAO's new state. Control messages 
("move one meter to the left") may be sent from the owner
to the NAO's copy at the server which may interpret them, change it's
(or some other NAO's) state and send syncs about the change to all clients.
</p>

SUBSECTION(NAO class with security enabled)
<p align=justify>
Here, <strong>only the server</strong> 
is allowed to create an NAO. The owner may still be
one of the clients. As above, copies of the NAO will spawn on all clients. 
<strong>Only the server</strong> is allowed to change the NAO and 
issue sync commands, sending sync messages to the clients.
The only way the NAO's owner has influence on it are the control messages
it may send to the server. 
</p><p align=justify>
The use of the security mode simply is: the server has full control over
the objects. Otherwise, people in an ego shooter could just teleport
themselves at will through the arena or make themselves invincible 
by simple modifications to the game (that's even an issue in closed
source games; look at Diabolo!).
</p>
SECTION(Usage)
<p align=justify>
A sample program defining a class of NAOs is 
<a href=../../src/network/l3_demo.cpp><CODE>l3_demo.cpp</CODE></a>
from the source directory. Compile it with <CODE>make
l3_demo</CODE>; the syntax is <CODE>l3_demo</CODE> to start it in 
server mode, just listening to clients, or <CODE>l3_demo
servername</CODE> to start it in client mode connecting to the server
given by <CODE>servername</CODE>; it will first show you how to
synchronize the NAO with sync messages, then how to control it with
control messages. Try connecting multiple clients to the server!
You'll see how easy it is; it's best to read this document and the 
sample program in parallel.
</p>

SUBSECTION(Overview)
To define a class of NAO's (let's call it <CODE>your_NAO</CODE>),
you derive a class from the base class
<CODE>netobject</CODE>  
and declare some member functions:

<ul>
<li>A normal <a href=#constr>constructor</a> and a virtual destructor.
</li>
<br><br>
<li>
For the <a href=#sync>synchronisation messages</a>,
 the send and receive functions
<pre>
virtual void write_sync(netmessage &m);
virtual void read_sync(netmessage &m);
</pre>
where <CODE>read_sync()</CODE> should read exactly the information
from <CODE>m</CODE> that <CODE>write_sync()</CODE> writes to it, and
if necessary a function deciding whether a sync message should be
accepted:
<pre>
virtual bool sync_is_new(netmessage &m);
</pre>
</li>
<li>
For <a href=#create>remote creation</a>, the send function
<pre>
virtual void write_create(netmessage &m); 
</pre>
and the remote constructor
<pre>
your_NAO(netmessage &m); 
</pre>
again reading exactly the information from <CODE>m</CODE> that 
<CODE>write_create()</CODE> wrote to it, and eventually a post-creation
function
<pre>
virtual void init_after_creation();
</pre>
being called after an object has been remotely created.
</li>
<br><br>
<li>
For the security system,
<pre>
virtual bool accept_client_sync() const;
</pre>
returning true if security is to be disabled. (Default: security is on.)
</li>
<br><br>
<li>
You need to create one object of the template class
<pre>
<a href=#identification>class net_initialisator&lt;your_NAO&gt;</a>;
</pre>
to give your class a unique identification across the network
(the constructor takes the name of your class as a string argument) and
the member function
<pre>
virtual netdescriptor &creator_descriptor() const;
</pre>
returning a reference to that object 
(<CODE>net_initialisator&lt;your_NAO&gt;</CODE>
is derived from the class <CODE>netdescriptor</CODE>).
</li>
<br><br>
<li>
And finally, if you want to use <a href=#control>control messages</a>, 
the receive function
<pre>
virtual void receive_control(netmessage &m);
</pre>
and of course some sending function.
</li>
</ul>

SUBSECTION(Details)

SUBSUBSECTION(<a name=constr>Constructor</a>)
<p align=justify>
The constructor
has to call call <CODE>netobject</CODE>'s constructor
</p><pre>
netobject::netobject(int owner=-1);
</pre><p align=justify>
where the argument is the user ID of the object's owner; leave it
blank or at <CODE>-1</CODE> if the creating computer itself should be 
the owner.
All in all, your constructor should look something like this:
</p><pre>
your_NAO::your_NAO(...) :netobject(owner) {
  // normal initialisation
  ...
}
</pre>

SUBSUBSECTION(<a name=sync>Synchronisation</a>)
<p align=justify>
Whenever you feel like your NAO's copies need to be synchronised with the
original (i.e. every time you change the original, or every .1
seconds), call the original's member function
</p><pre>
void request_sync(bool ack=true);
</pre><p align=justify>
(inherited form <CODE>netobject</CODE>). 
<CODE>ack</CODE> determines whether the sync message is guaranteed to
arrive; If the synchronisation is not vital 
(like updates of a constantly changing position where it is not fatal 
if one update is missed), you can set
<CODE>ack</CODE> to false. 
To really send the sync messages, you need to call the static function
</p><pre>
netobject::sync_all();
</pre><p align=justify>
every once in a while (best immediately before <CODE>receive()</CODE> from 
layer two). Shortly after your call of <CODE>request_sync()</CODE>, during
one of your calls to <CODE>netobject::sync_all();</CODE>
the network subsystem will call your original's member 
function <CODE>your_NAO::write_sync(netmessage&nbsp;&m)</CODE>
and broadcast the message <CODE>m</CODE>. 
When the other computers receive <CODE>m</CODE>, they
will simply call your NAO's copy's member function 
<CODE>your_NAO::read_sync(netmessage&nbsp;&m)</CODE>.
So your sync functions should read something like 
</p><pre>
virtual void write_sync(netmessage &m){
   // proper heritage:
   netobject::write_sync(m);

   // write all the possibly changing  
   // information form your object to m:
   m &lt;&lt; ...
}

virtual void read_sync(netmessage &m){
   // heritage:
   netobject::read_sync(m);
   
   // read the information exactly in the same 
   // order as it was written in write_sync:
   m &gt;&gt; ....
}
</pre><p align=justify>
If you detect an error during your <CODE>read_sync()</CODE>, caused by
the message being in a wrong [format], feel free to  kick the message's 
sender by throwing a <CODE>killhim</CODE>-exception.
</p><p align=justify>
Since sync messages may get lost or arrive in the wrong order, it is not
a good idea to write just the information that really changed in 
<CODE>write_sync()</CODE>; if you decide to do that kind of
bandwidth optimisation anyway, think exactly about what you are doing!
</p><p align=justify>
What happens now if a sync message is lost, sent again, lost again....
and receives the other computers way too late? Without protection measurements,
this will i.e. cause your racing car to be set back on the track until the
next sync packet arrives, correcting the mistake. This is not fatal, but
disturbing, and something needs to be done. Therefore, before calling
your NAO's <CODE>read_sync()</CODE>, the network subsystem calls
your NAO's member function
</p><pre>
virtual bool sync_is_new(netmessage &m);
</pre><p align=justify>
where you can read out <CODE>m</CODE> just like in <CODE>read_sync()</CODE>
and do some checks whether you really wish to accept the sync; i.e. with
every <CODE>write_sync</CODE>, you could include a time stamp (you should do
that anyway in a real time game) <CODE>sync_is_new()</CODE> may check;
if the timestamp is too old, you should reject the message. Only if
<CODE>sync_is_new()</CODE> returns <CODE>true</CODE>, <CODE>read_sync()</CODE>
is called. As the class <CODE>netobject</CODE> already implements a rudimentary
check in <CODE>netobject::sync_is_new(netmessage &m)</CODE>, guaranteeing
that each accepted sync message is newer than the one before,
you do not need to write your own check in most cases. If you do, it should
look like
</p><pre>
virtual bool sync_is_new(netmessage &m){
   // heritage:
   if (!netobject::sync_is_new(m))
     return false;	

   // your own checks, reading EXACTLY
   // the same information as read_sync()
   // (important for derived classes)
   m &gt;&gt; ....
   return result;
}
</pre>

SUBSUBSECTION(<a name=create>Remote creation</a>)
<p align=justify>
What happens now if you create a NAO with the
<a href=#constr>normal constructor</a>?
During one of the next calls of <CODE>netobject::sync_all()</CODE>,
remote creation messages will be sent to the other computers. They contain
all the information of a sync message, plus a bit more:
The sync messages are only intended to transport the part of the
NAO's information that is changing during the game; other parts of
the information may be fixed, i.e. the object's name, or a character's
race in a RPG. This information has to be written only once, and it would
be a waste of bandwidth to transmit it with every sync message. Therefore,
you should write all this fixed information in your NAO's member function
</p><pre>
virtual void write_create(netmessage &m){
   // again: heritage
   netobject::write_create(m);

   // your fixed information
   m &lt;&lt; ....
}
</pre><p align=justify>
So, when preparing a remote creation message,
<CODE>netobject::sync_all()</CODE> first lets your NAO write it's fixed
information with <CODE>write_create()</CODE> to it, then the changing
information with <CODE>write_sync()</CODE>. After that the message is
broadcasted and guaranteed to be received by the other computers.
They will then call your remote constructor which should look like
</p><pre>
your_NAO(netmessage &m)
:netobject(m) // heritage
{
  // read the fixed information
  m &gt;&gt; ....
}
</pre><p align=justify>
and after that your NAO's <CODE>read_sync()</CODE>. Since you may
need some of the variable information read by that function to completely
initiate your NAO, it's member function
</p><pre>
virtual void init_after_creation();
</pre><p align=justify>
is called after that where you can finish the construction. Again, if
an error occurs, kick the message's sender by throwing a 
<CODE>killhim</CODE>-exception.
</p>
SUBSUBSECTION(<a name=identification>Identification</a>)
<p>No big deal here: just write
</p><pre>
static net_initialisator&lt;your_NAO&gt; your_NAO_init("your_NAO");
</pre><p align=justify>
somewhere in your code file, declare the member function
</p><pre>
virtual netdescriptor &creator_descriptor() const;
</pre><p align=justify>
and define it as
</p><pre>
netdescriptor &your_NAO::creator_descriptor() const{
  return your_NAO_init;
}
</pre><p align=justify>
That's it. You have to repeat that for every NAO class you define.
</p>
SUBSUBSECTION(<a name=control>Control messages</a>)
<p align=justify>
Control messages can go from the owner of the NAO (a client) to the
server only. Do with them what you want, but they are mainly intended
to transport the user input to the server. Before sending a control
message, you'll have to create it using your NAO's member function
(inherited from <CODE>netobject</CODE>)
</p><pre>
 netmessage *new_control_message();
</pre><p align=justify>
Then, write to it whatever you want and send it to the server. The
server will call your NAO's member function
</p><pre>
virtual void receive_control(netmessage &m);
</pre><p align=justify>
which can then read and interpret the message. Of course, if you do
any changes to your NAO, you should call <CODE>request_sync()</CODE>
at the end. It is advisable to encapsulate the sending
process in an own member function (as
<CODE>new_control_message</CODE> has protected heritage, you are
forced to do that :-) ). As you can see, you have much freedom here
(and are basically on your own).
</p>

SUBSECTION(Pointers to netobjects)
<p align=justify>
<strong>NOTE: This section is still subject to change. Many of the
things you have to do manually now will be automated in future versions
of PROGNAME.</strong>
</p><p align=justify>
You will come to a point where you define a class of NAOs containing
pointers to other NAOs that need to be transmitted. The first problem:
</p>
SUBSUBSECTION(Transferring pointers)
<p align=justify>
Obviously, you can't just transfer them like integers. Instead of
writing a pointer to a NAO to a netmessage, simply write it's ID with
</p><pre>
m &lt;&lt; object->my_id();
</pre><p align=justify>
when receiving the id, it can be retransformed to a pointer with
</p><pre>
unsigned short id;
m &gt;&gt; id;
netobject *obj=netobject::object(id);
</pre><p align=justify>
In case the netobject with ID <CODE>id</CODE> has not yet been created,
<CODE>netobject::object(id)</CODE> will wait for it to spawn; with a
bit of luck, it will be created remotely shortly. That brings us to
the next problem: what if we're not lucky? There may easily be lockups
if i.e. the server waits for one of the client's NAOs to spawn, while
the client is waiting for some other message from the server (of
course, there is a timeout in <CODE>netobject::object()</CODE>.
But after that timeout, the connection is closed). An alternative
routine doing about the same job is <CODE>netobject</CODE>'s 
static member function
</p><pre>
static netobject *object_dangerous(int id);
</pre><p align=justify>
If the NAO labelled <CODE>id</CODE> does not exist, it will simply
return <CODE>NULL</CODE>.
</p>
SUBSUBSECTION(Waiting for NAOs to spawn remotely)
<p align=justify>
Sometimes, before you send a network message transferring a pointer to
a NAO, you want to make sure the NAO has been created remotely at the
message's receiver before sending the message; that avoids the
problems mentioned above. The NAO's member function
</p><pre>
bool has_been_transmitted(int user) const;
</pre><p align=justify>
(inherited form <CODE>netobject</CODE>) does exactly this check. 
For example, such checks should be done before a NAO depending on
another NAO (like, the rider of a horse...) is created remotely: you
want to be absolutely sure the horse is there before the rider
arrives. For exactly this situation, you can use your NAO's
member function 
</p><pre>
virtual bool clear_to_transmit(int user) const;
</pre><p align=justify>
It should return false if the NAO is not yet ready to be remotely
created at the computer with user ID <CODE>user</CODE>.
In our example, the rider's function should be defined as
</p><pre>
bool rider::clear_to_transmit(int user) const{
  return netobject::clear_to_transmit() && // heritage, as always...
         horse->has_been_transmitted(user);
}
</pre>
SUBSUBSECTION(Reference counters)
<p align=justify>
Another problem arises with remote destruction: It is to be avoided
that a NAO is destroyed while there are still pointers to
it. Therefore, each NAO has a reference counter you need to set
manually: call the member function
</p><pre>
void reg();
</pre><p align=justify>
every time you set a pointer to a NAO, and
</p><pre>
void unreg();
</pre><p align=justify>
every time you delete the pointer or let it point elsewhere.
</p>

include(sig.m4)
include(navbar.html.m4)

</body>
</html>





