include(head.html.m4)
define(SECTION,lower)
include(navbar.html.m4)

TITLE(Layer one: protocol abstraction)
<p align=justify>
Skip this layer if you really want to use Armagetron's networking system
fast; it contains details you do not need to know from the start.
</p>
<p align=justify>
This layer is responsible for sending raw data packets to other computers.
The files <CODE>net_anet.h,net_sysdep.C,net_udp.C</CODE> and
<CODE>net_wins.C</CODE> are responsible for it;
they are written in pure C.
<CODE>net_anet.h</CODE> is the header file with the declarations visible to
layer two, and <CODE>net_systep.C</CODE> just includes the real
implementation from <CODE>net_udp.C</CODE> or <CODE>net_wins.C</CODE> depending
on the system Armagetron is compiled for. 
</p>

SECTION(Data structures)

CODELISTBEGIN
CODEITEM(sockaddr,struct sockaddr:,
[Everything the network protocol needs to identify
a connection to another computer. In the case of UDP, it contains the IP 
address and port number.])

CODEITEM(socket,Sockets:,
[represented by integers: in analogy to file descriptors, 
all network IO is done
through sockets. Every client has one socket where all the messages to/from
the server are sent/received through. The server has one listening socket for
the messages from new clients, and one IO socket for each client.
(With the UDP protocol, all these sockets are really identical, as there is 
no such thing as a connection and no need/possibility to assign a connection 
to the sockets.)])
CODELISTEND

SECTION(Function list)
(only the functions layer two actually uses are mentioned)

CODELISTBEGIN
CODEITEM(init,Socket ANET_Init();,[Initialises the network system, returns the
control socket. All future IO is done through that socket.])

CODEITEM(shutdown,void ANET_Shutdown (void);,Closes the network system.)

CODEITEM(listen,void ANET_Listen(bool state);,[If state is true, the listening
socket is initialised (listening at the port ]<CODE>net_hostport=4532</CODE>[)
 and can be queried by]
<CODE>ANET_CheckNewConnections</CODE>.)

CODEITEM(checknewconnections,Socket ANET_CheckNewConnections (void);,
[On the server, this checks if there are any new connections
from clients wanting to join the game. Returns a socket ready to receive
the newcomer if there is one, or -1 if not. In case of the UDP protocol,
even the messages from the known clients are caught by this function 
(as there is no way to distinguish new and old connections with UDP), so layer
two needs to do some extra checks with this function.
])

CODEITEM(getaddrfromname,[int ANET_GetAddrFromName
(const char *name, struct sockaddr *addr);],[
Fills <CODE>*addr</CODE> with the information necessary to reach the server
<CODE>name</CODE> (containing the IP address in numerical notation or the 
hostname);
the port number is set to ]<CODE>net_hostport=4532</CODE> to match the port
the server listens on.)

CODEITEM(connect,ANET_Connect(Socket s,struct sockaddr *addr);,[
Opens a connection to the computer given by ]<CODE>addr</CODE>[ through
the socket] <CODE>s</CODE>. 
[As UDP is a connectionless protocol, this does not do
anything currently.])

CODEITEM(write,int  ANET_Write (Socket sock, const byte *buf, int len, struct sockaddr *addr);,
Sends <CODE>len</CODE> bytes of data from <CODE>buf</CODE> through the socket
<CODE>sock</CODE> to the peer identified with <CODE>*addr</CODE>. A connection
has to be established before with <CODE>ANET_Connect()</CODE>.)

CODEITEM(read,int  ANET_Read (Socket sock, byte *buf, int len, struct sockaddr *addr);,
Reads up to <CODE>len</CODE> bytes from the connection associated to 
socket <CODE>sock</CODE> and stores them in the buffer <CODE>buf</CODE>. 
[(If a connectionless protocol like UDP is used and the socket is a listening
socket, this can mean ANY data coming in at the port the socket listens on...)
]The sender's address is stored in <CODE>*addr</CODE>[, the number of actually
read bytes is returned (Zero means no data was received.)])

CODEITEM(addrtostring,char *ANET_AddrToString (const struct sockaddr *addr);,[
Returns the information from ]<CODE>addr</CODE>[ in human readable form.])

CODEITEM(addrcompare,int ANET_AddrCompare (struct sockaddr *addr1, struct sockaddr *addr2);,
Checks whether <CODE>*addr1</CODE> and <CODE>*addr2</CODE> [are the same 
computer (ignoring the port number). If they are, 0 is returned, -1 if not.])

CODEITEM(getsocketport,int  ANET_GetSocketPort (struct sockaddr *addr),
Returns the port number from <CODE>*addr</CODE>.)

CODELISTEND


include(sig.m4)
include(navbar.html.m4)

</body>
</html>

