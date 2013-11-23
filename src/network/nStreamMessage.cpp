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
#include "tMath.h"

#include "nBinary.h"

#include "utf8.h"

static nVersionFeature sn_ZeroMessageCrashfix( 1 );

#ifdef DEBUG
nMessageBase* sn_WatchMessage = NULL;
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
//! just received.
unsigned long int sn_ExpandMessageID( unsigned short id, unsigned short sender )
{
#ifdef DEBUG
    sn_BreakOnMessageID( id );
#endif

    static nMessageIDExpander expanders[MAXCLIENTS+2];

    tASSERT( sender <= MAXCLIENTS+2 )
    return expanders[sender].ExpandMessageID(id);
}

nStreamMessage::nStreamMessage( nStreamDescriptor const & descriptor, unsigned int messageID, unsigned int senderID )
: nMessageBase( descriptor, messageID ), readOut(0), descriptor_( descriptor )
{
    senderID_ = senderID;
}

nStreamMessage::nStreamMessage( nStreamDescriptor const & descriptor )
: nMessageBase( descriptor ), readOut(0), descriptor_( descriptor )
{
}

void nStreamMessage::OnRead( unsigned char const * & buffer, unsigned char const * end )
{
    nBinaryReader reader( buffer, end );    

    messageIDBig_ = sn_ExpandMessageID( reader.ReadShort(),senderID_);

    tRecorderSync< unsigned long >::Archive( "_MESSAGE_ID_IN", 3, messageIDBig_ );
    tRecorderSync< unsigned short >::Archive( "_MESSAGE_DECL_IN", 3, descriptorID_ );

    unsigned short len = reader.ReadShort();
    if ( len * 2 > end - buffer )
    {
        nReadError( true );
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

nStreamMessage& nStreamMessage::operator << (const tString &ss){
    tString s = ss;

    // convert utf8 to latin1. For comments the operator >>.
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
                utf8::uint32_t c = utf8::next( reader, s.end() );
                
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
    {
        // yep. Signed arithmetic. That gives
        // nice overflows. By the time we noticed,
        // it was too late to change :)
        signed char lo = s[i];
        signed char hi = s[i+1];

        // combine the two into a single short
        Write( short(lo) + (short(hi) << 8) );
    }

    // write last byte
    if (i<len)
        Write( static_cast< signed char >( sRaw[i] ) );

    return *this;
}

nStreamMessage& nStreamMessage::operator << (const tColoredString &s){
    return *this << static_cast< const tString & >( s );
}

nStreamMessage& nStreamMessage::operator << ( const tOutput &o ){
    return *this << tString( static_cast< const char * >( o ) );
}

static void sn_AddToString( tString & s, tString::CHAR c )
{
    if ( c )
        s += c;
}

nStreamMessage& nStreamMessage::ReadRaw(tString &s )
{
    s.Clear();
    unsigned short w,len;
    Read(len);
    if ( len > 0 )
    {
        s.reserve(len);
        for(int i=0;i<len;i+=2){
            Read(w);
            
            // carefully reverse the signed
            // encoding logic
            signed char lo = w & 0xff;
            signed short hi = ((short)w) - lo;
            hi >>= 8;
            sn_AddToString( s, lo );

            if (i+1<len)
                sn_AddToString( s, hi );
        }
    }

    return *this;
}

extern bool sn_filterColorStrings;
extern bool sn_filterDarkColorStrings;

nStreamMessage& nStreamMessage::operator >> (tColoredString &s )
{
    // read the raw data
    ReadRaw( s );

    // convert latin1 encoding to utf8
    {
        s =  st_Latin1ToUTF8(s);
    }

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

nStreamMessage& nStreamMessage::operator >> (tString &s )
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


nStreamMessage& nStreamMessage::operator<<(const REAL &x){


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

nStreamMessage& nStreamMessage::operator>>(REAL &x){
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
    if (!isfinite(x))
        st_Breakpoint();
    // con << " , x= " << x << '\n';
#endif
#endif
    return *this;
}

nStreamMessage& nStreamMessage::operator<< (const short &x){
    Write((reinterpret_cast<const short *>(&x))[0]);

    return *this;
}

nStreamMessage& nStreamMessage::operator>> (short &x){
    Read(reinterpret_cast<unsigned short *>(&x)[0]);

    return *this;
}

nStreamMessage& nStreamMessage::operator<< (const int &x){
    unsigned short a=x & (0xFFFF);
    short b=(x-a) >> 16;

    Write(a);
    operator<<(b);

    return *this;
}

nStreamMessage& nStreamMessage::operator>> (int &x){
    unsigned short a;
    short b;

    Read(a);
    operator>>(b);

    x=(b << 16)+a;

    return *this;
}

nStreamMessage& nStreamMessage::operator<< (const bool &x){
    if (x)
        Write(1);
    else
        Write(0);

    return *this;
}

nStreamMessage& nStreamMessage::operator>> (bool &x){
    unsigned short y;
    Read(y);
    x= (y!=0);

    return *this;
}


void nStreamMessage::Read(unsigned short &x){
    if (End()){
        tOutput o;
        st_Breakpoint();
        o.SetTemplateParameter(1, senderID_);
        o << "$network_error_shortmessage";
        con << o;
        // sn_DisconnectUser(senderID, "$network_kill_error");
        nReadError( false );
    }
    else
        x=data(readOut++);
}

//! handle this message
void nStreamMessage::OnHandle()
{
    tASSERT( dynamic_cast< nStreamDescriptor const * >( &GetDescriptor() ) );
    (* static_cast< nStreamDescriptor const & >( GetDescriptor() ).handler)( *this );
}

//! fills the receiving buffer with data
//!  @return descriptor ID to fill in
int nStreamMessage::OnWrite( WriteArguments & arguments ) const
{
    nBinaryWriter writer( arguments.buffer_ );

    // write message ID
    writer.WriteShort( MessageID() );

    // reserve space for data length
    nBinaryWriter dataLenWriter( writer );
    writer.WriteShort( 0 );

#ifdef DEBUG
    int overhead = arguments.buffer_.Len();
#endif

    // write raw data
    int len = DataLen();
    for(int i=0;i<len;i++)
    {
        writer.WriteShort( Data(i) );
    }

    tASSERT( 2 * len == arguments.buffer_.Len() - overhead );

    // determine data lenght and write it to reserved space
    
    dataLenWriter.WriteShort( len );

    return DescriptorID();
}

//! returns the descriptor
nDescriptorBase const & nStreamMessage::DoGetDescriptor() const
{
    return descriptor_;
}
