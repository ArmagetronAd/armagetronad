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

typedef void nHandler( nStreamMessage &m );

// old descriptor for streaming messages
class nStreamDescriptor: public nDescriptorBase
{
    friend class nStreamMessage;

    nHandler *handler;  // function responsible for our type of message

public:
    nStreamDescriptor(unsigned short identification,nHandler *handle,
                      const char *name, bool acceptEvenIfNotLoggedIn = false);

    ~nStreamDescriptor();

    //! creates a message for sending
    nStreamMessage * CreateMessage() const;
private:
    //! creates a message
    virtual nMessageBase * DoCreateMessage() const;
};

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

    nDescriptorBase const & descriptor_;

    ~nStreamMessage();
public:
    //! bends the descriptor around. useful for compatibility hacks.
    void BendDescriptorID( int descriptor )
    {
        descriptorID_ = descriptor;
    }

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

    nStreamMessage( const nStreamDescriptor &, unsigned int messageID, unsigned int senderID );  // create a new message

    explicit nStreamMessage( const nStreamDescriptor & );  // create a new message

    void Write(const unsigned short &x){
        // can't write to a reading message, or one that was finalized
        tASSERT( 0 == readOut );

        data[data.Len()]=x;
    }

    //! create a message from a pattern buffer
    // template< class MESSAGE >
    // static nMessageBase * Transform( MESSAGE const & message );

    nStreamMessage& operator<< (const REAL &x);
    nStreamMessage& operator>> (REAL &x);

    nStreamMessage& operator<< (const unsigned short &x){
        Write(x);
        return *this;
    }
    nStreamMessage& operator>> (unsigned short &x){
        Read(x);
        return *this;
    }

    nStreamMessage& operator<< (const double &x){
        return operator<<(REAL(x));
    }

    nStreamMessage& operator>> (double &x){
        REAL y;
        operator>>(y);
        x=y;

        return *this;
    }

    // read a string without any kind of filtering
    nStreamMessage& ReadRaw(tString &s);

    nStreamMessage& operator >> (tString &s);
    nStreamMessage& operator >> (tColoredString &s);
    nStreamMessage& operator << (const tString &s);
    nStreamMessage& operator << (const tColoredString &s);
    nStreamMessage& operator << (const tOutput &o);

    template<class T> void BinWrite (const T &x){
        for (unsigned int i=0;i<sizeof(T)/2;i++)
            Write((reinterpret_cast<const unsigned short *>(&x))[i]);
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
    }


    nStreamMessage& operator<< (const short &x);
    nStreamMessage& operator>> (short &x);

    nStreamMessage& operator<< (const int &x);
    nStreamMessage& operator>> (int &x);

    nStreamMessage& operator<< (const unsigned int &x){
        operator<<(reinterpret_cast<const int&>(x));
        return *this;
    }
    nStreamMessage& operator>> (unsigned int &x){
        operator>>(reinterpret_cast<int&>(x));
        return *this;
    }

    nStreamMessage& operator<< (const bool &x);
    nStreamMessage& operator>> (bool &x);

    template<class T> nStreamMessage& operator << (const T* p)
    {
        if (p)
            Write( p->ID() );
        else
            Write(0);

        return *this;
    }

    template<class T> nStreamMessage& operator << (const tControlledPTR<T> p)
    {
        if (p)
            Write( p->ID() );
        else
            Write(0);

        return *this;
    }

    inline nDescriptorBase const & GetDescriptor() const
    {
        return descriptor_;
    }
protected:
    //! handle this message
    virtual void OnHandle();

    //! fills the receiving buffer with data
    //!  @return descriptor ID to fill in
    virtual int OnWrite( WriteArguments & arguments ) const;

    //! reads data from network buffer
    virtual void OnRead( unsigned char const * & buffer, unsigned char const * end );

    //! returns the descriptor
    virtual nDescriptorBase const & DoGetDescriptor() const;
};

// read/write operators for versions
nStreamMessage& operator >> ( nStreamMessage& m, nVersion& ver );
nStreamMessage& operator << ( nStreamMessage& m, const nVersion& ver );

#endif
