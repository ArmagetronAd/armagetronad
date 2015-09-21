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

Although it is already included in the GPL, let me clarify:
anybody, especially the Krawall Gaming network, is allowed
to run a server/master server build from modified versions of
this file WHITHOUT releasing the source to the public (provided
the executable is not distributed).


*/

#include "nKrawall.h"
#include "nNetwork.h"
#include "nServerInfo.h"
#include "tString.h"
#include "tArray.h"
#include "tConsole.h"
#include "tSysTime.h"
#include "tMemManager.h"
#include "tRandom.h"

#include <stdlib.h>
#include <string>

bool nKrawall::MayRequirePassword(tString& adress, unsigned int port)
{
    return true;
    // TODO: Check for krawall adress

    if (adress.Len() < 4)
        return false;

    if (!strncmp(adress, "127.", 4))
        return true;

    return false;
}

bool nKrawall::ArePasswordsEqual(const nScrambledPassword& a,
                                 const nScrambledPassword& b)
{
    for (int i=15; i>=0; i--)
        if (a[i] != b[i])
            return false;

    return true;
}



// network read/write operations of these data types
void nKrawall::WriteScrambledPassword(const nScrambledPassword& scrambled,
                                      nMessage &m)
{
    for (int i = 7; i>=0; i--)
        m.Write(scrambled[i << 1] + (scrambled[(i << 1) + 1] << 8));
}

void nKrawall::ReadScrambledPassword( nMessage &m,
                                      nScrambledPassword& scrambled)
{
    for (int i = 7; i>=0; i--)
    {
        unsigned short x;
        m.Read(x);
        unsigned char low  = x & 255;
        unsigned char high = (x - low) >> 8;

        scrambled[ i << 1     ] = low;
        scrambled[(i << 1) + 1] = high;
    }
}

// network read/write operations of these data types
void nKrawall::WriteScrambledPassword(const nScrambledPassword& scrambled,
                                      std::ostream &s)
{
    for (int i = 15; i>=0; i--)
        s << (int)scrambled[i] << ' ';
}

void nKrawall::ReadScrambledPassword( std::istream &s,
                                      nScrambledPassword& scrambled)
{
    for (int i = 15; i>=0; i--)
    {
        int x;
        s >> x;
        scrambled[i] = x;
    }
}



// scramble a password locally (so it does not have to be stored on disk)
void nKrawall::ScramblePassword(const tString& username,
                                nScrambledPassword &scrambled)
{
    md5_state_t state;
    md5_init(&state);
    md5_append(&state, (md5_byte_t const *)(username.c_str()), username.Len());
    md5_finish(&state, scrambled);
}


// scramble it again before transfering it over the network
void nKrawall::ScrambleWithSalt(const nScrambledPassword& source,
                                const nSalt& salt,
                                nScrambledPassword& dest)
{
    md5_state_t state;
    md5_init(&state);
    md5_append(&state, source, 16);
    md5_append(&state, salt  , 16);
    md5_finish(&state, dest);
}









#ifdef KRAWALL_SERVER

// called on the master server when the league message is received
void nKrawall::ReceiveLeagueMessage(const tString& message)
{
    //  con << message;

    if (message[0] == 'K')
    {
        tString killer(&message[1]);
        tString victim(&message[1+killer.Len()]);

        MasterFrag(killer, victim);
    }
    else if (message[0] == 'R')
    {
        tString numP(&message[1]);
        int pos = 1 + numP.Len();

        int numPlayers = atoi(numP);
        tArray<tString> players(numPlayers);
        for (int i = numPlayers-1; i>=0; i--)
        {
            players(i) = tString(&message[pos]);
            pos += players(i).Len();
        }

        MasterRoundEnd(&players[0], numPlayers);
    }
}



// called when victim drives against killer's wall
void nKrawall::Frag(const tString &killer, const tString& victim, tString& message)
{
    message << 'K';
    message << killer << '\0';
    message << victim << '\0';
}

// called at the end of a round; the last survivor is stored in
// players[numPlayers-1], the first death in players[0]
void nKrawall::RoundEnd(const tString* players, int numPlayers, tString& message)
{
    message << 'R';
    message << numPlayers << '\0';
    for (int i = numPlayers-1; i>=0; i--)
        message << players[i] << '\0';
}



// get a random salt value
void nKrawall::RandomSalt(nSalt& salt)
{
    // oh dear. getting a random salt value with this method is EVIL...
    tRandomizer & randomizer = tRandomizer::GetInstance();
    for (int i=15; i>=0; i--)
        salt[i] = randomizer.Get( 256 );
    //        salt[i] = (int)(256.0 * rand() / (RAND_MAX + 1.0));
}


// called ON THE SERVER when victim drives against killer's wall
void nKrawall::ServerFrag(const tString &killer, const tString& victim)
{
    tString message;
    Frag(killer, victim, message);
    SendLeagueMessage(message);
}

// called ON THE SERVER at the end of a round; the last survivor is stored in
// players[numPlayers-1], the first death in players[0]
void nKrawall::ServerRoundEnd(const tString* players, int numPlayers)
{
    tString message;
    RoundEnd(players, numPlayers, message);
    SendLeagueMessage(message);
}


// league messages are sent without network connection, so we need to add acks manually

void ReceiveLeagueMessage(nMessage &m);
void ReceiveLeagueMessageAck(nMessage &m);

static nDescriptor nLeagueMessage(42, &ReceiveLeagueMessage, "password_request", true);

static nDescriptor nLeagueMessageAck(43, &ReceiveLeagueMessageAck, "password_answer", true);


// league security
static void SignMessage(unsigned int id, const tString& message, nKrawall::nScrambledPassword& signature)
{
    tString pw = message;
    pw << " " << id;
    nKrawall::nScrambledPassword temp;
    nKrawall::ScramblePassword(pw, temp);
    nKrawall::ScrambleWithSalt(temp, nKrawall::SecretLeagueKey(), signature);
}


static unsigned int S_NextID = 3;

// league messages
class nLM
{
public:
    tString        message;
    unsigned int id;
    REAL           sentTime;
};


// the league messages still waiting for an ack
static tArray<nLM> S_WFA;


// called on the servers to create a league message
void nKrawall::SendLeagueMessage(const tString& message)
{
    // z-man: disabled for now, we want no central league messages sent to my master
    return;

    int i;
    REAL time = tSysTimeFloat();

    // bend the network port to the master server
    nServerInfo *master = nServerInfo::GetMasters();
    if (!master)
        return;

    sn_Bend(master->GetConnectionName(), master->GetPort());

    // Resend old messages
    for (i=0; i < S_WFA.Len(); i++)
    {
        nLM& resend = S_WFA(i);
        if (resend.sentTime + 2 < time)
        {
            // resend the mesage
            nMessage *m = tNEW(nMessage) (nLeagueMessage);
            (*m) << resend.id;
            (*m) << resend.message;
            // sign the message
            nKrawall::nScrambledPassword signature;
            SignMessage(resend.id, resend.message, signature);
            nKrawall::WriteScrambledPassword(signature, *m);

            m->SendImmediately(0, false);

            // update send time
            resend.sentTime = time;
        }
    }

    if (!&message)
        return;

    // make a new ack-waiting entry and fill it
    nLM& send = S_WFA[S_WFA.Len()];
    send.id = S_NextID++;
    send.sentTime = time;
    send.message  = message;



    // sign the message
    nKrawall::nScrambledPassword signature;
    SignMessage(send.id, message, signature);

    // pack a message and go
    nMessage *m = tNEW(nMessage) (nLeagueMessage);
    (*m) << send.id;
    (*m) << message;
    nKrawall::WriteScrambledPassword(signature, *m);

    m->SendImmediately(0, false);
    nMessage::SendCollected(0);
}














#define STOREBACK 40

// league messages
class nLastLeagueMessage
{
public:
    tString        adr;
    unsigned int   port;
    unsigned int   ids[STOREBACK];
};


// the league messages we last got
static tArray<nLastLeagueMessage> S_LLM;




void ReceiveLeagueMessage(nMessage &m)
{
    int i;

    // get the adress of the sender
    tString      senderAdr;
    sn_GetAdr(m.SenderID(), senderAdr);
    unsigned int senderPort = sn_GetPort(m.SenderID());

    unsigned int id;
    tString message;

    // read the message
    m >> id;
    m >> message;

    // do nothing if the message is from an unknown location
    if (!nKrawall::IsFromKrawall(senderAdr, senderPort))
    {
        con << "Rejecting league message " << id << " from " << senderAdr << ":" << senderPort << " : not from Krawall.\n";
        return;
    }

    // return an ack
    nMessage *ret = tNEW(nMessage)(nLeagueMessageAck);
    (*ret) << id;
    ret->SendImmediately(m.SenderID(), false);
    nMessage::SendCollected(m.SenderID());

    // check the signature
    nKrawall::nScrambledPassword realsignature, receivedsignature;
    SignMessage(id, message, realsignature);
    nKrawall::ReadScrambledPassword(m, receivedsignature);
    if (!nKrawall::ArePasswordsEqual(realsignature, receivedsignature))
    {
        con << "Rejecting league message " << id << " from " << senderAdr << ":" << senderPort << " : invalid signature.\n";
        return;
    }

    // find/create the nLastLeagueMessage entry
    nLastLeagueMessage* lastFromThisSender = NULL;
    for (i=S_LLM.Len()-1; i>=0 && !lastFromThisSender; i--)
        if (S_LLM(i).adr == senderAdr && S_LLM(i).port == senderPort)
            lastFromThisSender = &(S_LLM(i));

    // not found: create it
    if (!lastFromThisSender)
    {
        lastFromThisSender = &(S_LLM[S_LLM.Len()]);
        lastFromThisSender->adr  = senderAdr;
        lastFromThisSender->port = senderPort;
        for (i = STOREBACK-1; i>=0; i--)
            lastFromThisSender->ids[i] = id - i - 10000;
    }



    // check if the id is new
    for (i = STOREBACK-1; i>=0; i--)
        if (lastFromThisSender->ids[i] == id)
        {
            con << "Ignoring league message " << id << " from " << senderAdr << ":" << senderPort << " : already processed.\n";
            return;
        }

    // store the ID
    for (i = STOREBACK-2; i>=0; i--)
        lastFromThisSender->ids[i+1] = lastFromThisSender->ids[i];
    lastFromThisSender->ids[0] = id;

#ifdef DEBUG
    con << "Receiving league message " << id << "\n";
#endif

    // evaluate it
    nKrawall::ReceiveLeagueMessage(message);
}







void ReceiveLeagueMessageAck(nMessage &m)
{
    // read the return
    unsigned int id;
    m >> id;

    for (int i=S_WFA.Len()-1; i>=0; i--)
    {
        nLM& resend = S_WFA(i);
        if (resend.id == id)
        {
            // delete it
            S_WFA(i) = S_WFA(S_WFA.Len()-1);
            S_WFA.SetLen(S_WFA.Len()-1);

            return;
        }
    }
}

#endif


