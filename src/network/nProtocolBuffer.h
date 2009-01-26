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

#ifndef ArmageTron_ProtocolBuffer_H
#define ArmageTron_ProtocolBuffer_H

// protocol buffers forward declaration
namespace google { namespace protobuf { class Message; } }
typedef google::protobuf::Message nProtoBuf;

namespace Network{ class nNetObjectInit; }

#include "nNetwork.h"
class nNetObject;
class nNetObjectRegistrar;

#include <map>

extern nVersionFeature sn_protocolBuffers;

//! fills a message with a protocol buffer
class nMessageFillerProtoBufBase: public nMessageFiller
{
protected:
    //! fills the receiving buffer with data
    virtual void OnFill( nSendBuffer::Buffer & buffer, int receiver ) const;

    //! returns the wrapped protcol buffer
    inline nProtoBuf const & GetMessage() const
    {
        return DoGetMessage();
    }

private:
    //! returns the wrapped protcol buffer
    virtual nProtoBuf const & DoGetMessage() const = 0;
};

//! implementation for each protocl buffer 
template< class PROTOBUF > 
class nMessageFillerProtoBuf: public nMessageFillerProtoBufBase
{
public:
    //! returns the wrapped protcol buffer
    inline PROTOBUF const & GetMessage() const
    {
        return protoBuf_;
    }

    //! returns the wrapped protcol buffer
    inline PROTOBUF & AccessMessage()
    {
        return protoBuf_;
    }
private:
    //! returns the wrapped protcol buffer
    virtual nProtoBuf const & DoGetMessage() const
    {
        return protoBuf_;
    }

    //! the wrapped buffer
    PROTOBUF protoBuf_;
};

//! extra information about the sender of a message
struct nSenderInfo
{
public:
    //! reference to the message, it contains all info
    nMessage const & envelope;
    
    int SenderID() const
    {
        return envelope.SenderID();
    }
    
    explicit nSenderInfo( nMessage const & e )
    : envelope( e )
    {}
};

// new descriptor for protocol buffer messages
class nPBDescriptorBase: public nDescriptorBase
{
public:
    typedef std::map< std::string, nPBDescriptorBase * > DescriptorMap;

    nPBDescriptorBase(unsigned short identification,
                      nProtoBuf const & prototype, bool acceptEvenIfNotLoggedIn = false);
    ~nPBDescriptorBase();

    static std::string const & DetermineName( nProtoBuf const & prototype );

    //! dumb streaming to message
    inline void StreamTo( nProtoBuf const & in, nMessage & out ) const
    {
        DoStreamTo( in, out );
    }

    //! dumb streaming from message
    inline void StreamFrom( nMessage & in, nProtoBuf & out  ) const
    {
        DoStreamFrom( in, out );
    }

    //! dumb streaming from message, static version
    static void StreamFromStatic( nMessage & in, nProtoBuf & out  );

    //! dumb streaming to message, static version
    static inline void StreamToStatic( nProtoBuf const & in, nMessage & out );

    //! selective writing to message, either embedded or transformed
    void WriteMessage( nProtoBuf const & in, nMessage & out ) const;

    //! selective reading message, either embedded or transformed
    void ReadMessage( nMessage & in, nProtoBuf & out  ) const;

    //! creates a protocol buffer of the managed type. Needs to be deleted later.
    inline nProtoBuf * Create() const
    {
        return DoCreate();
    }

    //! compares two messages, filling in total size and difference.
    static void EstimateMessageDifference( nProtoBuf const & a,
                                           nProtoBuf const & b,
                                           int & total,
                                           int & difference,
                                           bool & removed );

    //! calculates the difference between two messages
    static void DiffMessages( nProtoBuf const & base,
                              nProtoBuf const & derived,
                              nProtoBuf & diff,
                              bool copy = true );

    //! clears all repeated fields from a message
    static void ClearRepeated( nProtoBuf & message );

    // the above three functions are supposed to be used for compression.
    // the sender first uses EsimateMessageDifference to  find a suitable previous
    // message  to reference, then calls Diffmessages in the two messages, sends
    // the difference message and a reference to base over to the receiver.
    // The receiver makes a copy of the base message, calls ClearRepeated onn it,
    // then merges the difference message into it.
    
    // flag that marks protocol buffer messages
    static const unsigned short protoBufFlag = 0x8000;
protected:
    static DescriptorMap const & GetDescriptorsByName();

private:
    static DescriptorMap descriptorsByName;

    //! dumb streaming to message
    virtual void DoStreamTo( nProtoBuf const & in, nMessage & out ) const;

    //! dumb streaming from message
    virtual void DoStreamFrom( nMessage & in, nProtoBuf & out ) const;

    //! dumb streaming from message, static version
    static void StreamFromDefault( nMessage & in, nProtoBuf & out  );

    //! dumb streaming to message, static version
    static void StreamToDefault( nProtoBuf const & in, nMessage & out );

    //! creates a protocol buffer of the managed tpye. Needs to be deleted later.
    virtual nProtoBuf * DoCreate() const = 0;
};

// read/write operators for protocol buffers
nMessage& operator >> ( nMessage& m, nProtoBuf & buffer );
nMessage& operator << ( nMessage& m, nProtoBuf const & buffer );

// templated version of protocol buffer messages
template< class PROTOBUF > 
class nPBDescriptor: public nPBDescriptorBase
{
    typedef void Handler( PROTOBUF & message, nSenderInfo const & sender );

    //! instance of this descriptor
    static nPBDescriptor * instance_;

#ifdef DEBUG
    bool multipleInstances_;
#endif

    //! function actually handling the incoming message
    Handler * handler_;

    //! delegates message handling
    virtual void DoHandleMessage( nMessage & envelope )
    {
        // read into protocol buffer
        PROTOBUF protocolBuffer;

        ReadMessage( envelope, protocolBuffer );

        // and delegate
        handler_( protocolBuffer, nSenderInfo( envelope ) );
    }

    //! creates a protocol buffer of the managed tpye. Needs to be deleted later.
    virtual nProtoBuf * DoCreate() const
    {
        return new PROTOBUF;
    }
public:
    //! puts a puffer into a message
    nMessage * Transform( PROTOBUF const & message ) const
    {
        nMessage * ret = new nMessage( *this );

        WriteMessage( message, *ret );

        return ret;
    }

    //! puts a puffer into a message
    static nMessage * TransformStatic( PROTOBUF const & message )
    {
        tASSERT( instance_ );

        // if there are multiple instances of descriptors of the same message class,
        // you can't pick one reliably.
        tASSERT( !instance_->multipleInstances_ );

        return instance_->Transform( message );
    }

    nPBDescriptor( unsigned short identification, Handler * handler,
                   bool acceptEvenIfNotLoggedIn = false )
    : nPBDescriptorBase( identification, PROTOBUF(), acceptEvenIfNotLoggedIn )
    , handler_( handler )
    {
#ifdef DEBUG
        multipleInstances_ = instance_;
#endif
        instance_ = this;
    }
};

//! instance of this descriptor
template< class PROTOBUF > 
nPBDescriptor< PROTOBUF > * nPBDescriptor< PROTOBUF >::instance_ = 0;

// create a message from a pattern buffer
template< class PROTOBUF >
nMessage * nMessage::Transform( PROTOBUF const & message )
{
    return nPBDescriptor< PROTOBUF >::TransformStatic( message );
}

// network object descriptors
class nOPBDescriptorBase
{
public:
    virtual ~nOPBDescriptorBase();

    //! creates an initialization message for an object
    inline nMessage * WriteInit( nNetObject & object ) const
    {
        return DoWriteInit( object );
    }

    //! writes sync message
    inline void WriteSync( nNetObject & object, nMessage & message ) const
    {
        return DoWriteSync( object, message );
    }

    //! reads a sync message
    inline void ReadSync( nNetObject & object, nMessage & envelope ) const
    {
        return DoReadSync( object, envelope );
    }
protected:
    // fetches the network object ID from a creation message
    template< class PROTOBUF >
    static unsigned int GetObjectID( PROTOBUF const & message )
    {
        return GetObjectID( message.base() );
    }

    static unsigned int GetObjectID ( Network::nNetObjectInit const & message );

    //! checks to run before creating a new object
    static bool PreCheck( unsigned short id, nSenderInfo sender );

    //! checks to run after creating a new object
    static void PostCheck( nNetObject * object, nSenderInfo sender );
private:
    //! creates an initialization message for an object
    virtual nMessage * DoWriteInit( nNetObject & object ) const = 0;

    //! writes a sync message
    virtual void DoWriteSync( nNetObject & object, nMessage & message ) const = 0;

    //! reads a sync message
    virtual void DoReadSync( nNetObject & object, nMessage & envelope ) const = 0;
};

//! specialization of descriptor for each network object class
template< class OBJECT, class PROTOBUF >
class nOPBDescriptor: public nOPBDescriptorBase, public nPBDescriptor< PROTOBUF >
{
public:
    nOPBDescriptor( int identification )
    : nOPBDescriptorBase()
    , nPBDescriptor< PROTOBUF >( identification, &HandleCreation, false )
    {}
private:
    // template message
    static const PROTOBUF prototype;

    static void HandleCreation( PROTOBUF & message, nSenderInfo const & sender )
    {
        unsigned short id = GetObjectID( message.init() );
        
        if( PreCheck( id, sender ) )
        {
            nNetObjectRegistrar registrar;
            tJUST_CONTROLLED_PTR< OBJECT > n=new OBJECT( message.init(), message.sync(), sender );
            n->InitAfterCreation();
            n->ReadSync( message.sync(), sender );
            n->Register( registrar );
            
            PostCheck( n, sender );
        }
    }

    //! writes sync, true work
    template< class SYNC >
    void DoWriteSync( nNetObject & object, nMessage & message, SYNC const & ) const
    {
        // cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );

        // write sync
        SYNC sync;
        cast.WriteSync( sync );

        // transfer sync to message
        WriteMessage( sync, message );
    }

    //! reads sync, true work
    template< class SYNC >
    void DoReadSync( nNetObject & object, nMessage & envelope, SYNC const & ) const
    {
        // cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );

        // read sync from message
        SYNC sync;

        // transfer sync to message
        ReadMessage( envelope, sync );

        // delegate to object
        cast.ReadSync( sync, nSenderInfo( envelope ) );
    }

    //! creates an initialization message for an object
    virtual nMessage * DoWriteInit( nNetObject & object ) const
    {
        // init message
        PROTOBUF message;
        
        // object cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );
        
        // work
        cast.WriteInit( *message.mutable_init() );
        cast.WriteSync( *message.mutable_sync() );
        
        // and transform
        return nMessage::Transform( message );
    }

    //! creates an sync message for an object
    virtual void DoWriteSync( nNetObject & object, nMessage & message ) const
    {
        DoWriteSync( object, message, prototype.sync() );
    }

    //! reads a sync message
    virtual void DoReadSync( nNetObject & object, nMessage & envelope ) const
    {
        DoReadSync( object, envelope, prototype.sync() );
    }
};

// template message
template< class OBJECT, class PROTOBUF >
const PROTOBUF nOPBDescriptor< OBJECT, PROTOBUF >::prototype;

class nMessageCache
{
};

#endif
