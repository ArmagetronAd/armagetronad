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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "config.h"
#include "tSafePTR.h"
#include <string>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <iostream>
#include "nNetObject.h"

class floattest: public nNetObject{
public:
    REAL x; // our changing state
    REAL y; // our fixed state

    // normal constructor
    floattest():nNetObject(),x(0),y(1.1){
        con << "created floattest.\n";
    }

    // remote constructor
    floattest(nMessage &m):nNetObject(m){
        m >> y;
        con << "created floattest by remote order from "
        << owner << " with fixed state " << y << ".\n";
        // as x is changing, it will be read by the following ReadSync()
    }


    virtual ~floattest(){
        con << "deleting floattest owned by " << owner << ".\n";
    }

    // disable security
    virtual bool AcceptClientSync() const{
        return true;
    }

    // synchronisation: just read and write our state x
    virtual void WriteSync(nMessage &m){
        nNetObject::WriteSync(m);
        m << x;
    }

    virtual void ReadSync(nMessage &m){
        nNetObject::ReadSync(m);

        m >> x;

        // print our state
        con << "NAO owned by " << owner << " is in state " << x << "\n";
    }

    // our creation write saving only the fixed info
    virtual void WriteCreate(nMessage &m){
        nNetObject::WriteCreate(m);
        m << y;
    }

    // control messages order the object to change it's state by
    // the transferred number:
    virtual void ReceiveControl(nMessage &m){
        nNetObject::ReadSync(m);

        REAL diff;
        m >> diff;
        x+=diff;

        // print our state
        con << "NAO owned by " << owner << " got order to change by "
        << diff<< "; state now is " << x << ".\n";

        RequestSync();
    }

    // send a control message
    void SendControl(REAL diff){
        // create the message
        nMessage *m=NewControlMessage();

        // write order
        (*m) << diff;

        // send message
        m->BroadCast();
    }

    // identification:
    virtual nDescriptor &CreatorDescriptor() const;
};

nNOInitialisator<floattest> floattest_init(100,"floattest");

nDescriptor &floattest::CreatorDescriptor() const{
    return floattest_init;
}
// identification end.




// demonstration how to handle pointers to other netobjects:
class deptest: public nNetObject{
public:
    tCONTROLLED_PTR(floattest) dep;

    // normal constructor
    deptest(floattest *d):nNetObject(),dep(d){
        con << "created deptest.\n";
    }

    // remote constructor
    deptest(nMessage &m):nNetObject(m){

        // read in dep's id:
        unsigned short id;
        m >> id;
        dep=(floattest *)Object(id); // find the object with the right ID

        con << "created deptest by remote order from "
        << owner << ".\n";
    }

    virtual ~deptest(){
        con << "deleting deptest owned by " << owner << ".\n";
    }

    // disable security
    virtual bool AcceptClientSync() const{
        return true;
    }

    // synchronisation: not needed

    // our creation write saving only the fixed info
    virtual void WriteCreate(nMessage &m){
        nNetObject::WriteCreate(m);
        m << dep->ID();
    }

    // identification:
    virtual nDescriptor &CreatorDescriptor() const;

    // transmission clearence:
    virtual bool ClearToTransmit(int user) const{
        return nNetObject::ClearToTransmit(user) &&
               dep->HasBeenTransmitted(user);
    }
};

nNOInitialisator<deptest> deptest_init(101,"deptest");

nDescriptor &deptest::CreatorDescriptor() const{
    return deptest_init;
}
// identification end.


unsigned short client_gamestate[MAXCLIENTS+2];

void server(){

    int loop=100000; // a long delay loop

    sn_SetNetState(nSERVER); // initialise server mode

    while(loop>0 && sn_GetNetState()!=nSTANDALONE){ // and loop a while
        sn_Receive();
        nNetObject::SyncAll();
        sn_SendPlanned();

        loop--;
        usleep(10000);
    }

    sn_SetNetState(nSTANDALONE); // exit.
    usleep(10000);
    sn_Receive();
    sn_SendPlanned();
}


void client(const tString &serv){
    tCONTROLLED_PTR(floattest) x=new floattest();
    tCONTROLLED_PTR(deptest)   y=new deptest(x);

    sn_Connect(serv); // initialise client state and connect to the given server

    con << "Demo 1: transfering state by sync messages\n";

    int loop=10; // loop only shortly
    while(loop>0 && sn_GetNetState()!=nSTANDALONE){
        nNetObject::SyncAll();
        sn_Receive();

        // change x's state and request a sync with the server
        con << "new state: " << (x->x+=1) << "\n";
        x->RequestSync();

        nNetObject::SyncAll();
        sn_SendPlanned();
        sn_Receive();
        sn_SendPlanned();

        loop--;
        usleep(1000000);
    }

    if (loop==0){

        con << "Demo 2: sending control messages. Note the delay in the response\n"
        << "caused by the network loop functions not beeing called "
        "often enough.\n";


        loop=10;
        while(loop>0 && sn_GetNetState()!=nSTANDALONE){
            nNetObject::SyncAll();
            sn_Receive();

            // send a control message, decreasing x's value
            x->SendControl(-1);

            nNetObject::SyncAll();
            sn_Receive();
            sn_SendPlanned();

            loop--;
            usleep(1000000);
        }

        // get rest of messages
        loop=100;
        while(loop>0 && sn_GetNetState()!=nSTANDALONE){
            sn_Receive();
            nNetObject::SyncAll();
            sn_SendPlanned();

            loop--;
            usleep(1000);
        }
    }

    if (loop==0)
        con << "Regular logout..\n";

    sn_SetNetState(nSTANDALONE);
    usleep(100000);
    sn_Receive();
    sn_SendPlanned();
}



int main(int argnum,char **arglist){
    if (argnum<=1){
        server();
    }
    else{
        client(arglist[1]);
    }
}
