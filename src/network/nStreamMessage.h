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

#ifndef ArmageTron_STREAM_MESSAGE_H
#define ArmageTron_STREAM_MESSAGE_H

#include "nNetwork.h"

// old style stream network messages
class nStreamMessage: public nMessageBase
{
    friend class nDescriptorBase;
    friend class nProtoBufDescriptorBase;
    friend class nNetObject;
    friend class nWaitForAck;

protected:
    tArray<unsigned short> data;  //!< assuming ints are 32 bit wide...

    unsigned int readOut;

    ~nStreamMessage();
public:
    unsigned short DataLen() const{
        return data.Len();
    }

    virtual int Size() const;

    unsigned short Data(unsigned short n) const {
        return data(n);
    }

    //! makes sure nothing else gets written to the message (in debug mode)
    void Finalize()
    {
#ifdef DEBUG
        readOut++;
#endif
    }

    nStreamMessage( const nDescriptorBase & );  // create a new message
    nStreamMessage(unsigned char * & buffer, short sn_myNetID, unsigned char * end );
    // read a message from the network stream

    void Write(const unsigned short &x){
        // can't write to a reading message, or one that was finalized
        tASSERT( 0 == readOut );

        data[data.Len()]=x;
    }

    //! create a message from a pattern buffer
    template< class MESSAGE >
    static nMessage * Transform( MESSAGE const & message );

    nMessage& operator<< (const REAL &x);
    nMessage& operator>> (REAL &x);

    nMessage& operator<< (const unsigned short &x){
        Write(x);
        return *this;
    }
    nMessage& operator>> (unsigned short &x){
        Read(x);
        return *this;
    }

    nMessage& operator<< (const double &x){
        return operator<<(REAL(x));
    }

    nMessage& operator>> (double &x){
        REAL y;
        operator>>(y);
        x=y;

        return *this;
    }

    // read a string without any kind of filtering
    nMessage& ReadRaw(tString &s);

    nMessage& operator >> (tString &s);
    nMessage& operator >> (tColoredString &s);
    nMessage& operator << (const tString &s);
    nMessage& operator << (const tColoredString &s);
    nMessage& operator << (const tOutput &o);

    template<class T> void BinWrite (const T &x){
        for (unsigned int i=0;i<sizeof(T)/2;i++)
            Write((reinterpret_cast<const unsigned short *>(&x))[i]);
        return *this;
    }

    bool End(){
        return readOut>=static_cast<unsigned int>(data.Len());
    }

    void Reset(){
        readOut=0;
    }

    int ReadSoFar(){
        return readOut;
    }

    void Read(unsigned short &x);

    template<class T> void BinRead (const T &x){
        for (unsigned int i=0;i<sizeof(T)/2;i++)
            Read(reinterpret_cast<unsigned short *>(&x)[i]);
        return *this;
    }


    nMessage& operator<< (const short &x);
    nMessage& operator>> (short &x);

    nMessage& operator<< (const int &x);
    nMessage& operator>> (int &x);

    nMessage& operator<< (const unsigned int &x){
        operator<<(reinterpret_cast<const int&>(x));
        return *this;
    }
    nMessage& operator>> (unsigned int &x){
        operator>>(reinterpret_cast<int&>(x));
        return *this;
    }

    nMessage& operator<< (const bool &x);
    nMessage& operator>> (bool &x);

    template<class T> nMessage& operator << (const T* p)
    {
        if (p)
            Write( p->ID() );
        else
            Write(0);

        return *this;
    }

    template<class T> nMessage& operator << (const tControlledPTR<T> p)
    {
        if (p)
            Write( p->ID() );
        else
            Write(0);

        return *this;
    }
};

// read/write operators for versions
nMessage& operator >> ( nMessage& m, nVersion& ver );
nMessage& operator << ( nMessage& m, const nVersion& ver );

std::istream& operator >> ( std::istream& s, nVersion& ver );
std::ostream& operator << ( std::ostream& s, const nVersion& ver );

#endif
