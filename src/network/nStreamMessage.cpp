/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "nStreamMessage.h"

#include "tRecorder.h"
#include "tConsole.h"
#include "tLocale.h"
#include "tConfiguration.h"

#include "nBinary.h"

#include "utf8.h"

static nVersionFeature sn_ZeroMessageCrashfix( 1 );

#ifdef DEBUG
nMessage* sn_WatchMessage = NULL;
unsigned int sn_WatchMessageID = 76;

void sn_BreakOnMessageID( unsigned int messageID )
{
    if (messageID == sn_WatchMessageID && messageID != 0 )
    {
        int x;
        x = 0;
    }
}
#endif

class nMessageIDExpander
{
    unsigned long quarters[4];
public:
    nMessageIDExpander()
    {
        for (int i=3; i>=0; --i)
            quarters[i]=i << 14;
    }

    unsigned long ExpandMessageID( unsigned short id )
    {
        // the current ID is in this quarter
        int thisQuarter = ( id >> 14 ) & 3;

        // the following quarter will be this
        int nextQuarter = ( thisQuarter + 1 ) & 3;

        // make sure the following quarter has a higher upper ID completion than this
        quarters[nextQuarter] = quarters[thisQuarter] + ( 1 << 14 );

        // replace high two bits of incoming ID with the counted up ID
        return quarters[thisQuarter] | id;
    }
};

//! expands a short message ID to a full int message ID, assuming it is from a message that was
// just received.
static unsigned long int sn_ExpandMessageID( unsigned short id, unsigned short sender )
{
#ifdef DEBUG
    sn_BreakOnMessageID( id );
#endif

    static nMessageIDExpander expanders[MAXCLIENTS+2];

    tASSERT( sender <= MAXCLIENTS+2 )
    return expanders[sender].ExpandMessageID(id);
}

nStreamMessage::nStreamMessage( nDescriptorBase const & descriptor )
: nMessageBase( descriptor ), readOut(0)
{
}

nStreamMessage::nStreamMessage(unsigned char const * & buffer,short sender, unsigned char const * end )
: readOut(0)
{
    nBinaryReader reader( buffer, end );    

    descriptor = reader.ReadShort();
    messageIDBig_ = sn_ExpandMessageID( reader.ReadShort(),sender);
    senderID = sender;

    tRecorderSync< unsigned long >::Archive( "_MESSAGE_ID_IN", 3, messageIDBig_ );
    tRecorderSync< unsigned short >::Archive( "_MESSAGE_DECL_IN", 3, descriptor );

    unsigned short len = reader.ReadShort();
    if ( len * 2 > end - buffer )
    {
        throw nKillHim();
    }
    for(int i=0;i<len;i++)
    {
        data[i]= reader.ReadShort();
    }

#ifdef DEBUG
    sn_BreakOnMessageID( messageIDBig_ );
#endif
}

nStreamMessage::~nStreamMessage()
{
}

int nStreamMessage::Size() const
{
    return DataLen() * 2;
}

nMessage& nStreamMessage::operator << (const tString &ss){
    tString s = ss;

    /*
    // Z-Man: commented out looking for a better way, please don't delete yet until we have a proper
    // different way of sending UTF8 over the network.

    // convert utf8 to latin1. For comments the operator >>.
    if ( sn_GetNetState() == nSERVER ? !ServerSendsUnicode() : !unicode.Supported(0) )
    */

    {
        try
        {
            tString copy;

            tString::iterator reader = s.begin();
            std::back_insert_iterator< tString > writer = back_inserter(copy);
            
            while ( reader != s.end() )
            {
                // just convert every byte of the original latin1 into utf8, assuming unicode matches latin1
                // where latin1 is defined.
                uint32_t c = utf8::next( reader, s.end() );
                
                // convert unknown characters to underscores
                if ( c < 256 )
                    *writer = c;
                else
                    *writer = '_';
                
                ++writer;
            }

            s = copy;
        }
        catch( ... )
        {
            // no need to do a thing. utf8 passes as latin1 almost all of the time, unless
            // you're German or French or anything non-English. well.
            con << "latin1 to utf8 conversion error!\n";
        }
    }

    if ( !sn_ZeroMessageCrashfix.Supported() && s.Len() <= 0 )
    {
        return this->operator<<( s + " " );
    }

    unsigned short len=s.Size()+1;

    // clamp away excess zeroes
    while(len > 1 && s(len-2)==0)
    {
        --len;
    }

    // check whether all clients support zero length strings
    if ( !sn_ZeroMessageCrashfix.Supported() )
    {
        if ( len <= 0 )
        {
            static tString replacement("");
            return this->operator<<( replacement );
        }
    }
    else if ( len == 1 )
    {
        // do away with the the trailing zero in zero length strings.
        len = 0;
    }

    Write(len);
    int i;

    char const * sRaw = s;

    // write first pairs of bytes
    for(i=0;i+1<len;i+=2)
        Write(sRaw[i]+(sRaw[i+1] << 8));

    // write last byte
    if (i<len)
        Write(sRaw[i]);

    return *this;
}

nMessage& nStreamMessage::operator << (const tColoredString &s){
    return *this << static_cast< const tString & >( s );
}

nMessage& nStreamMessage::operator << ( const tOutput &o ){
    return *this << tString( static_cast< const char * >( o ) );
}

static void sn_AddToString( tString & s, tString::CHAR c )
{
    if ( c )
        s += c;
}

nMessage& nStreamMessage::ReadRaw(tString &s )
{
    s.Clear();
    unsigned short w,len;
    Read(len);
    if ( len > 0 )
    {
        s.reserve(len);
        for(int i=0;i<len;i+=2){
            Read(w);
            tString::CHAR c1 = w & 255;
            sn_AddToString( s, c1 );
            if (i+1<len)
                sn_AddToString( s, (w-c1) >> 8 );
        }
    }

    return *this;
}

bool sn_filterColorStrings = false;
static tConfItem<bool> sn_filterColorStringsConf("FILTER_COLOR_STRINGS",sn_filterColorStrings);
bool sn_filterDarkColorStrings = false;
static tConfItem<bool> sn_filterDarkColorStringsConf("FILTER_DARK_COLOR_STRINGS",sn_filterDarkColorStrings);

nMessage& nStreamMessage::operator >> (tColoredString &s )
{
    // read the raw data
    ReadRaw( s );

    // convert latin1 encoding to utf8
    // The server knows which clients support unicode precisely; every unicode supporting client
    // sends in utf8 to a unicode supporting server. However, the server has to send back latin1
    // to all clients once one client does not support unicode, because messages containing strings
    // can be broadcast. So the client has to do the conversion if only one client is online
    // that does not support unicode.
    // if ( sn_GetNetState() == nSERVER ? !unicode.Supported( SenderID() ) : !ServerSendsUnicode() )
    {
        s =  st_Latin1ToUTF8(s);
    }
    /*
    // Z-Man: commented out looking for a better way, please don't delete yet until we have a proper
    // different way of sending UTF8 over the network.
    else
    {
        // the incoming string is supposed to be in utf8 format. check that, and crop it otherwise.
        // no invalid utf8 string is supposed to be able to enter the system. Maybe it's just a
        // stray latin1 string? If not, it's set to an error string.
        if ( !utf8::is_valid( s.begin(), s.end() ) )
            s = st_Latin1ToUTF8(s);
    }
    */

    // filter client string messages
    if ( sn_GetNetState() == nSERVER )
    {
        s.NetFilter();
        s.RemoveTrailingColor();
    }

    // filter color codes away
    if ( sn_filterColorStrings )
        s = tColoredString::RemoveColors( s, false );
    else if ( sn_filterDarkColorStrings )
        s = tColoredString::RemoveColors( s, true );

    return *this;
}

nMessage& nStreamMessage::operator >> (tString &s )
{
    tColoredString safe;
    operator>>( safe );
    s = safe;

    return *this;
}


#define MANT 26
#define EXP (32-MANT)
#define MS (MANT-1)


typedef struct{
int mant:MANT;
unsigned int exp:EXP;
} myfloat;


nMessage& nStreamMessage::operator<<(const REAL &x){


#ifdef DEBUG
    // con << "write x= " << x;


    if(sizeof(myfloat)!=sizeof(int))
        tERR_ERROR_INT("floating ePoint format does not work!");
#endif
    /*
      REAL nachkomma=x-floor(x);
      Write(short(x-nachkomma));
      Write(60000*nachkomma);
    */
    // no fuss. Read and write floats in binary format.
    // will likely cause problems for systems other than i386.

    //Write(((short *)&x)[0]);
    //Write(((short *)&x)[1]);

    // right. Caused severe problems with the AIX port.

    // new way: own floating ePoint format that is not good with small numbers
    // (we do not need them anyway)
    REAL y=x;

    unsigned int negative=0;
    if (y<0){
        y=-y;
        negative=1;
    }

    unsigned int exp=0;
    while ( fabs(y)>=64 && exp < (1<<EXP)-6 )
    {
        exp +=6;
        y/=64;
    }
    while ( fabs(y)>=1 && exp < (1<<EXP)-1 )
    {
        exp++;
        y/=2;
    }
    // now x=y*2^exp
    unsigned int mant=int(y*(1<<MS));
    // now x=mant*2^exp * (1/ (1<<MANT))

    // cutoffs:
    if (mant>((1<<MS))-1)
        mant=(1<<MS)-1;

    if (exp>(1<<EXP)-1){
        exp=(1<<EXP)-1;
        if (mant>0)
            mant=(1<<MS)-1;
    }

    // put them together:

    unsigned int trans=(mant & ((1<<MS)-1)) | (negative << MS) | (exp << MANT);
    /*
      myfloat trans;
      trans.exp=exp;
      trans.mant=mant;
    */

    operator<<(reinterpret_cast<int &>(trans));

#ifdef DEBUG
    /*
      con << "mant: " << mant
      << ", exp: " << exp
      << ", negative: " << negative;
    */

    unsigned int mant2=trans & ((1 << MS)-1);
    unsigned int negative2=(trans >> MS) & 1;
    unsigned int nt=trans-mant-(negative << MS);
    unsigned int exp2=nt >> MANT;

    if (mant2!=mant || negative2!=negative || exp2!=exp)
        tERR_ERROR_INT("Floating ePoint tranfer failure!");

    /*
      con << ", x: " << x;

      con << ", mant: " << mant
      << ", exp: " << exp
      << ", negative: " << negative;
    */

    // check:

    REAL z=REAL(mant)/(1<<MS);
    if (negative)
        z=-z;

    while (exp>=6){
        exp-=6;
        z*=64;
    }
    while (exp>0){
        exp--;
        z*=2;
    }

    if (fabs(z-x)>(fabs(x)+1)*.001)
        tERR_ERROR_INT("Floating ePoint tranfer failure!");

    //con << ", z: " << z << '\n';
#endif

    return *this;
}

nMessage& nStreamMessage::operator>>(REAL &x){
    /*
      short vorkomma;
      unsigned short nachkomma;
      Read((unsigned short &)vorkomma);
      Read(nachkomma);
      x=vorkomma+nachkomma/60000.0;

      Read(((unsigned short *)&x)[0]);
      Read(((unsigned short *)&x)[1]);
     */

    unsigned int trans;
    operator>>(reinterpret_cast<int &>(trans));

    int mant=trans & ((1 << MS)-1);
    unsigned int negative=(trans >> MS) & 1;
    unsigned int exp=(trans-mant-(negative << MS)) >> MANT;

    x=REAL(mant)/(1<<MS);
    if (negative)
        x=-x;

#ifdef DEBUG
    //  con << "read mant: " <<mant << ", exp: " << exp;
#endif

    while (exp>=6){
        exp-=6;
        x*=64;
    }
    while (exp>0){
        exp--;
        x*=2;
    }

#ifdef DEBUG
#ifndef WIN32
    if (!finite(x))
        st_Breakpoint();
    // con << " , x= " << x << '\n';
#endif
#endif
    return *this;
}

nMessage& nStreamMessage::operator<< (const short &x){
    Write((reinterpret_cast<const short *>(&x))[0]);

    return *this;
}

nMessage& nStreamMessage::operator>> (short &x){
    Read(reinterpret_cast<unsigned short *>(&x)[0]);

    return *this;
}

nMessage& nStreamMessage::operator<< (const int &x){
    unsigned short a=x & (0xFFFF);
    short b=(x-a) >> 16;

    Write(a);
    operator<<(b);

    return *this;
}

nMessage& nStreamMessage::operator>> (int &x){
    unsigned short a;
    short b;

    Read(a);
    operator>>(b);

    x=(b << 16)+a;

    return *this;
}

nMessage& nStreamMessage::operator<< (const bool &x){
    if (x)
        Write(1);
    else
        Write(0);

    return *this;
}

nMessage& nStreamMessage::operator>> (bool &x){
    unsigned short y;
    Read(y);
    x= (y!=0);

    return *this;
}


void nStreamMessage::Read(unsigned short &x){
    if (End()){
        tOutput o;
        st_Breakpoint();
        o.SetTemplateParameter(1, senderID);
        o << "$network_error_shortmessage";
        con << o;
        // sn_DisconnectUser(senderID, "$network_kill_error");
        nReadError( false );
    }
    else
        x=data(readOut++);
}
