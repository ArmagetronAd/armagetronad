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
#include "nNetwork.h"
#include "nNet.h"
#include <string>
#include <iostream>

#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> 
#else
#include <windows.h>
#endif

#include "nNetObject.h"

class floattest;
class deptest;

List<floattest> floattests;

class floattest: public nNetObject{
    int listID;

public:
    REAL x;
    deptest *dep;

    virtual void debug_out(){
        con << id << ":" << owner << ":" << x;
    }

    floattest():nNetObject(),listID(-1),x(-4){
        dep=NULL;
        con << "created floattest.\n";
        floattests.Add(this,listID);
    }
    floattest(nMessage &m):nNetObject(m),listID(-1){
        dep=NULL;
        //con << "created floattest.\n";
        floattests.Add(this,listID);
    }

    virtual ~floattest(){
        if(dep){
            tERR_ERROR("floattest deleted before deptest!");
        }

        floattests.Remove(this,listID);
        con << "deleted floattest.\n";
    }

    virtual bool AcceptClientSync() const{
        return true;
    }

    virtual void WriteSync(nMessage &m){
        nNetObject::WriteSync(m);
        m << x;
    }

    virtual void ReadSync(nMessage &m){
        nNetObject::ReadSync(m);
        m >> x;
    }

    virtual nDescriptor &CreatorDescriptor() const;
};

nNOInitialisator<floattest> floattest_init(200,"floattest");

nDescriptor &floattest::CreatorDescriptor() const{
    return floattest_init.desc;
}

List<deptest, false, true> deptests;

class deptest: public nNetObject{
    int listID;

public:
    floattest *dep;

    void debug_out(){
        con << "x:";
        dep->debug_out();
    }

    deptest(floattest *x):listID(-1),dep(x){
        con << "created deptest.\n";
        dep->dep=this;
        deptests.Add(this,listID);
    }

    deptest(nMessage &m):nNetObject(m),listID(-1){
        con << "creating deptest.\n";
        unsigned short id;
        m.Read(id);
        dep=(floattest *)nNetObject::Object(id);
        if (dep->dep){
            tERR_ERROR("two deptests for the same same floattest!");
        }

        dep->dep=this;
        deptests.Add(this,listID);
    }

    virtual ~deptest(){
        dep->dep=NULL;
        con << "deleted deptest.\n";
        deptests.Remove(this,listID);
    }

    virtual bool ClearToTransmit(int user) const{
        return dep->HasBeenTransmitted(user);
    }

    virtual void WriteCreate(nMessage &m){
        nNetObject::WriteCreate(m);
        m.Write(dep->ID());
    }

    virtual bool AcceptClientSync() const{
        return true;
    }

    virtual nDescriptor &CreatorDescriptor() const;
};

nNOInitialisator<deptest> deptest_init("deptest");

nDescriptor &deptest::CreatorDescriptor() const{
    return deptest_init.desc;
}


int ntest(const tString &serv,bool server){
    floattest x;
    deptest *y=NULL;



    int i=100000;
    if (server){
        sn_SetNetState(nSERVER);
        //y=new deptest(&x);
    }
    else{
        sn_Connect(serv);
        i=1000;
    }


    while(i>0 && sn_GetNetState()!=nSTANDALONE){
        sn_Receive();
        nNetObject::SyncAll();
        sn_SendPlanned();

        x.x+=.007;
        if (i%10==0){
            x.RequestSync();
            if (sn_GetNetState()==nCLIENT || i%100==0){
                for(int j=floattests.Len()-1;j>=0;j--){
                    floattests[j]->debug_out();
                    con << '\t';
                }
                con << '\n';
            }
        }

        i--;
        usleep(10000);

        /*
        static int lastprint=0;

        if (current_id%10==0 && current_id!=lastprint){
          lastprint=current_id;
          con << "netid=" << current_id << '\n';
        }
        */
    }

    if (i==0)
        con << "Regular logout..\n";

    sn_SetNetState(nSTANDALONE);
    usleep(100000);
    sn_Receive();
    sn_SendPlanned();

    if (y)
        delete y;

    return 0;
}


