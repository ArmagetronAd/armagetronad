/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#include "config.h"
#include "tConsole.h"
#include "tLocale.h"
#include "defs.h"
#include <string>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <iostream>
#include "nNetwork.h"

#define MAX 70000

bool Transmitted[MAX];

void LoginLogout()
{
    con << "LoginLogout()\n";

    if ( nSERVER != sn_GetNetState() )
        return;

    bool failed = false;

    if ( !nCallbackLoginLogout::Login() )
    {
        con << "Client quit; checking messages.\n";
        for ( int i = MAX-1; i>=0; --i )
            if ( !Transmitted[i] )
            {
                con << "Message " << i << " has not been transmitted.\n";
                failed = true;
                i-=MAX/10;
            }
    }

    if ( failed )
        exit(-1);

    for ( int i = MAX-1; i>=0; --i )
        Transmitted[i] = false;
}

static nCallbackLoginLogout loginlogout(LoginLogout);

// a sample message receiver: print all the shorts stored in m.
void sample_message_handler(nMessage &m){
    int i=1;
    while(!m.End()){
        short out;
        m >> out;
        if ( out < 20 )
        {
            con << i << " :\t" << out << '\n';
            i++;
        }

        Transmitted[out] = true;
    }
}



// the descriptor
nDescriptor message_test(31,&sample_message_handler,"message_test");

void sample_message_handler2(nMessage &m){
    while(!m.End()){
        unsigned int out;
        m >> out;
        Transmitted[out] = true;
    }
}



// the descriptor
nDescriptor message_test2(32,&sample_message_handler2,"message_test2");

unsigned short client_gamestate[MAXCLIENTS+2];


void server(){
    int loop=100000; // a long delay loop

    sn_SetNetState(nSERVER); // initialise server mode

    while(loop>0 && sn_GetNetState()!=nSTANDALONE){ // and loop a while
        sn_Receive();
        loop--;
        usleep(10000);
    }

    sn_SetNetState(nSTANDALONE); // exit.
    usleep(10000);
    sn_Receive();
}


void client(const tString &serv){
    sn_Connect(serv); // initialise client state and connect to the given server

    if (sn_GetNetState()!=nCLIENT){
        con << "Login failed.\n";
        return;
    }

    nMessage *m;
    int i;

    /*
    con << "\n\nFirst, we send a single short (17) to the server:\n";

    m=new nMessage(message_test);
    (*m) << (short)17;
    m->BroadCast();
    sn_Receive();

    usleep(3000000);

    con << "\n\nThen, we try three shorts(10,1,7):\n";

    m=new nMessage(message_test);
    (*m) << (short)10;
    (*m) << (short)1;
    (*m) << (short)7;
    m->BroadCast();
    sn_Receive();

    usleep(3000000);

    con << "\n\nLet's see what happens if we forget the type cast\n"
        <<  "and send our number (17) as int:\n";

    m=new nMessage(message_test);
    (*m) << 17;
    m->BroadCast();
    sn_Receive();

    // wait before we exit, getting messages and repeating lost ones
    for(i=300;i>=0;i--){
      sn_Receive();
      usleep(1000);
    }

    con << "\nAha. Thought so :-> It arrives as two shorts.\n\n\n";
    */
    con << "\nSending all numbers from zero to " << MAX-1 << ":\n\n\n";

    for ( unsigned int j = 0; j < MAX; ++j )
    {
        // don't let too many messages be pending
        while( sn_MessagesPending(0) > 10){
            sn_Receive();
            usleep(1000);
        }

        m=new nMessage(message_test2);
        (*m) << j;
        m->BroadCast();
        sn_Receive();
    }

    // wait before we exit, getting messages and repeating lost ones
    for(i=300;i>=0  || sn_MessagesPending(0) > 0; i--){
        con << "Pending messages: " << sn_MessagesPending(0) << "\n";
        sn_Receive();
        usleep(1000);
    }

    sn_SetNetState(nSTANDALONE);
    usleep(1000);
    sn_Receive();
}



int main(int argnum,char **arglist){
    if (argnum<=1){
        server();
    }
    else{
        client(arglist[1]);
    }

    tLocale::Clear();
}
