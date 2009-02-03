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

#include "nProtoBufForward.h"

// protocol buffers forward declaration
namespace google { namespace protobuf { class Message; class Descriptor; } }
typedef google::protobuf::Message nProtoBuf;

namespace Network{ class nNetObjectInit; }

#include "tMemManager.h"

#include "nNetwork.h"
#include "nNetObject.h"

class nNetObject;
class nNetObjectRegistrar;

class nBinaryReader;
class nBinaryWriter;

#include <map>
#include <deque>
// #include <hash_map>

extern nVersionFeature sn_protoBuf;

class nProtoBufDescriptorBase;
template< class PROTOBUF >
class nProtoBufDescriptor;

//! header structure of a ProtoBuf message
struct nProtoBufHeader
{
    unsigned int   len;        //! the length of the buffer
    unsigned short messageID;  //! message ID
    unsigned short cacheRef;   //! reference to cache base message

    nProtoBufHeader()
    : len(0), messageID(0), cacheRef(0)
    {}

    // read/write functions
    void Write( nBinaryWriter & writer );
    void Read ( nBinaryReader & reader );
};

class nMessageStreamer;

//! protocol buffer message
class nProtoBufMessageBase: public nMessageBase
{
public: 
    nProtoBufMessageBase( nProtoBufDescriptorBase const & descriptor );

    //! set the compatibility streamer
    void SetStreamer( nMessageStreamer * streamer )
    {
        streamer_ = streamer;
    }

    //! gets the compatibility streamer
    nMessageStreamer * GetStreamer() const
    {
        return streamer_;
    }
    
    //! returns the wrapped protcol buffer
    inline nProtoBuf const & GetProtoBuf() const
    {
        return DoGetProtoBuf();
    }

    //! returns the wrapped protcol buffer
    inline nProtoBuf & AccessProtoBuf()
    {
        return DoAccessProtoBuf();
    }

    //! returns a temporary work protocol buffer
    inline nProtoBuf & AccessWorkProtoBuf() const
    {
        return DoAccessWorkProtoBuf();
    }

    //! returns the descriptor
    nProtoBufDescriptorBase const & GetDescriptor() const;
protected:
    //! fills the receiving buffer with data
    virtual int OnWrite( WriteArguments & arguments ) const;

    //! reads data from network buffer
    virtual void OnRead( unsigned char const * & buffer, unsigned char const * end );
private:
    // approximate size of the message in bytes
    virtual int Size() const;

    //! returns the wrapped protcol buffer
    virtual nProtoBuf const & DoGetProtoBuf() const = 0;

    //! returns the wrapped protcol buffer
    virtual nProtoBuf & DoAccessProtoBuf() = 0;

    //! returns the wrapped work protcol buffer
    virtual nProtoBuf & DoAccessWorkProtoBuf() const = 0;

    //! dummy message used to cache raw data in old message format
    mutable tJUST_CONTROLLED_PTR< nStreamMessage > oldFormat_;

    //! the compatibility streamer
    nMessageStreamer * streamer_;
};

//! implementation for each protocol buffer type
template< class PROTOBUF > 
class nProtoBufMessage: public nProtoBufMessageBase
{
public:
    explicit nProtoBufMessage( nProtoBufDescriptor< PROTOBUF > const & descriptor )
    : nProtoBufMessageBase( descriptor )
    , descriptor_( descriptor )
    {
    }

    //! returns the wrapped protcol buffer
    inline PROTOBUF const & GetProtoBuf() const
    {
        return protoBuf_;
    }

    //! returns the wrapped protcol buffer
    inline PROTOBUF & AccessProtoBuf()
    {
        return protoBuf_;
    }

    //! returns the wrapped protcol buffer
    inline static PROTOBUF & AccessWorkProtoBuf()
    {
        return workProtoBuf_;
    }

    inline nProtoBufDescriptor< PROTOBUF > const & GetDescriptor() const
    {
        return descriptor_;
    }
protected:
    //! handles the message
    virtual void OnHandle()
    {
        nSenderInfo senderInfo( *this );
        GetDescriptor().Handle( GetProtoBuf(), senderInfo );
    }

private:
    //! returns the wrapped protcol buffer
    virtual nProtoBuf const & DoGetProtoBuf() const
    {
        return protoBuf_;
    }

    //! returns the wrapped protcol buffer
    virtual nProtoBuf & DoAccessProtoBuf()
    {
        return protoBuf_;
    }

    //! returns the wrapped work protcol buffer
    virtual nProtoBuf & DoAccessWorkProtoBuf() const
    {
        return workProtoBuf_;
    }

    //! returns the descriptor
    virtual nProtoBufDescriptor< PROTOBUF > const & DoGetDescriptor() const
    {
        return descriptor_;
    }

    //! the descriptor
    nProtoBufDescriptor< PROTOBUF > const & descriptor_;

    //! the wrapped buffer
    PROTOBUF protoBuf_;

    //! the wrapped work buffer
    static PROTOBUF workProtoBuf_;
};

template< class PROTOBUF >
PROTOBUF nProtoBufMessage< PROTOBUF >::workProtoBuf_;

//! extra information about the sender of a message
struct nSenderInfo
{
public:
    //! reference to the message, it contains all info
    nMessageBase const & envelope;
    
    int SenderID() const
    {
        return envelope.SenderID();
    }

    int MessageIDBig() const
    {
        return envelope.MessageIDBig();
    }

    unsigned short MessageID() const
    {
        return envelope.MessageID();
    }
    
    explicit nSenderInfo( nMessageBase const & e )
    : envelope( e )
    {}
};

//! class converting messages for older clients
class nMessageTranslatorBase
{
public:
    nMessageTranslatorBase();

    //! constructor registering with the descriptor
    explicit nMessageTranslatorBase( nProtoBufDescriptorBase & descriptor );
    
    //! convert current message format to format suitable for old client
    virtual nMessageBase * Translate( nProtoBuf const & source, int receiver ) const;
};

// new descriptor for protocol buffer messages
class nProtoBufDescriptorBase: public nDescriptorBase
{
public:
    typedef std::map< std::string, nProtoBufDescriptorBase * > DescriptorMap;

    //! sections to stream in each protobuf. Sections are delimitered by
    //! elements with IDs past the range of reserved IDs (>= 20000)
    enum StreamSections
    {
        SECTION_First = 1,  // only the first section (default behavior, normal messages only have one section)
        SECTION_ID     = 2, // only the object ID from sync messages
        SECTION_Second = 4, // only the second section
        SECTION_All    = 5, // both sections for creation message
        SECTION_Default = 1,
        SECTION_Max     = 7
    };

    nProtoBufDescriptorBase(unsigned short identification,
                      nProtoBuf const & prototype, bool acceptEvenIfNotLoggedIn = false);
    ~nProtoBufDescriptorBase();

    //! sets the compatibility streamer used by messages of this class
    void SetStreamer( nMessageStreamer * streamer )
    {
        streamer_ = streamer;
    }

    //! gets the compatibility streamer used by messages of this class
    nMessageStreamer * GetStreamer() const
    {
        return streamer_;
    }

    inline nMessageTranslatorBase * GetTranslator() const
    {
        return translator_;
    }

    inline void SetTranslator(nMessageTranslatorBase * translator )
    {
        translator_ = translator;
    }

    nMessageStreamer & GetDefaultStreamer();

    static std::string const & DetermineName( nProtoBuf const & prototype );

    //! dumb streaming to message
    inline void StreamTo( nProtoBuf const & in, nStreamMessage & out, StreamSections sections ) const
    {
        for ( int flag = 1; flag < SECTION_Max; flag <<= 1 )
        {
            if ( sections & flag )
            {
                DoStreamTo( in, out, StreamSections(flag) );
            }
        }
    }

    //! dumb streaming from message
    inline void StreamFrom( nStreamMessage & in, nProtoBuf & out, StreamSections sections ) const
    {
        for ( int flag = 1; flag < SECTION_Max; flag <<= 1 )
        {
            if ( sections & flag )
            {
                DoStreamFrom( in, out, StreamSections(flag) );
            }
        }
    }

    //! dumb streaming from message, static version
    static void StreamFromStatic( nStreamMessage & in, nProtoBuf & out, StreamSections sections );

    //! dumb streaming to message, static version
    static void StreamToStatic( nProtoBuf const & in, nStreamMessage & out, StreamSections sections );

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

    //! creates a message
    nProtoBufMessageBase * CreateMessage() const
    {
        return DoCreateMessage();
    }

protected:
    static DescriptorMap const & GetDescriptorsByName();

private:
    static DescriptorMap descriptorsByName;

    //! gets the compatibility streamer used by messages of this class
    nMessageStreamer * streamer_;

    //! translator for old clients
    nMessageTranslatorBase * translator_;

    //! creates a message
    virtual nProtoBufMessageBase * DoCreateMessage() const = 0;

    //! dumb streaming to message
    virtual void DoStreamTo( nProtoBuf const & in, nStreamMessage & out, StreamSections sections ) const;

    //! dumb streaming from message
    virtual void DoStreamFrom( nStreamMessage & in, nProtoBuf & out, StreamSections sections ) const;

    //! dumb streaming from message, static version
    static void StreamFromDefault( nStreamMessage & in, nProtoBuf & out, StreamSections sections );

    //! dumb streaming to message, static version
    static void StreamToDefault( nProtoBuf const & in, nStreamMessage & out, StreamSections sections );
};

// network object descriptors
class nOProtoBufDescriptorBase
{
public:
    virtual ~nOProtoBufDescriptorBase();

    //! writes sync message
    inline nProtoBufMessageBase * WriteSync( nNetObject & object, bool create ) const
    {
        return DoWriteSync( object, create );
    }

    //! writes sync message, old style (for subclasses)
    inline void WriteSync( nNetObject & object, nStreamMessage & stream, bool create ) const
    {
        return DoWriteSync( object, stream, create );
    }

    //! reads from old style sync message
    inline void ReadSync( nNetObject & object, nStreamMessage & stream, bool create ) const
    {
        return DoReadSync( object, stream, create );
    }

    //! streamer for creation messages
    static nMessageStreamer * CreationStreamer();

    //! streamer for sync messages
    static nMessageStreamer * SyncStreamer();

protected:
    // fetches the network object ID from a creation message
    template< class PROTOBUF >
    static unsigned int GetObjectID( PROTOBUF const & message )
    {
        tASSERT( message.has_base() );
        return GetObjectID( message.base() );
    }

    static unsigned int GetObjectID ( Network::NetObjectSync const & message );

    //! checks to run before creating a new object
    static bool PreCheck( unsigned short id, nSenderInfo sender );

    //! checks to run after creating a new object
    static void PostCheck( nNetObject * object, nSenderInfo sender );

private:
    //! writes a sync message
    virtual nProtoBufMessageBase * DoWriteSync( nNetObject & object, bool create ) const = 0;

    //! reads an old sync message
    virtual void DoReadSync( nNetObject & object, nStreamMessage & envelope, bool create ) const = 0;

    //! writes into an old sync message
    virtual void DoWriteSync( nNetObject & object, nStreamMessage & envelope, bool create ) const = 0;

    //! creates a message
    // virtual nProtoBufMessageBase * DoCreateMessage() const = 0;
};


//! class converting protobuf messages to stream messages
class nMessageStreamer
{
public:
    nMessageStreamer();

    //! constructor setting the default streamer of the given descriptor
    explicit nMessageStreamer( nProtoBufDescriptorBase & descriptor );
    virtual ~nMessageStreamer();

    //! function responsible for turning message into old stream message
    virtual void StreamFromProtoBuf( nProtoBufMessageBase const & source, nStreamMessage & target ) const;

    //! function responsible for reading an old message
    virtual void StreamToProtoBuf( nStreamMessage & source, nProtoBufMessageBase & target ) const;
};

//! class converting messages for older clients
template< class PROTOBUF >
class nMessageTranslator: public nMessageTranslatorBase
{
public:
    //! constructor registering with the descriptor
    explicit nMessageTranslator( nProtoBufDescriptor< PROTOBUF > & descriptor )
    : nMessageTranslatorBase( descriptor ){}

    //! convert current message format to format suitable for old client
    virtual nMessageBase * Translate( nProtoBuf const & source, int receiver ) const
    {
        tASSERT( dynamic_cast< PROTOBUF const * >( &source ) );
        return Translate( static_cast< PROTOBUF const & >( source ), receiver );
    }

    //! convert current message format to format suitable for old client
    virtual nMessageBase * Translate( PROTOBUF const & source, int receiver ) const
    {
        return NULL;
    }
};

// templated version of protocol buffer messages
template< class PROTOBUF > 
class nProtoBufDescriptor: public nProtoBufDescriptorBase
{
public:
    typedef void Handler( PROTOBUF const & message, nSenderInfo const & sender );

    //! creates and schedules a message for broadcast (in client mode, that means just sending it to the server), returning the protovuf to fill
    PROTOBUF & Broadcast( bool ack = true )
    {
        nProtoBufMessage< PROTOBUF > * m = CreateMessage();
        m->BroadCast( ack );
        return m->AccessProtoBuf();
    }

    //! creates and schedules a message for broadcast to all clients supporting a certain feature, returning the protovuf to fill
    PROTOBUF & Broadcast( nVersionFeature const & feature, bool ack = true )
    {
        nProtoBufMessage< PROTOBUF > * m = CreateMessage();
        lastSent_ = m;
        m->BroadCast( feature, ack );
        return m->AccessProtoBuf();
    }

    //! creates and schedules a message for sending to a specific peer, returning the protovuf to fill
    PROTOBUF & Send( unsigned int receiver, bool ack = true )
    {
        nProtoBufMessage< PROTOBUF > * m = CreateMessage();
        lastSent_ = m;
        m->Send( receiver, ack );
        return m->AccessProtoBuf();
    }

    //! puts a puffer into a message
    nMessageBase * Transform( PROTOBUF const & protoBuf ) const
    {
        // pack buffer into a message filler
        nProtoBufMessage< PROTOBUF > * m = CreateMessage();
        lastSent_ = m;
        m->AccessProtoBuf() = protoBuf;
        
        return m;
    }

    //! puts a puffer into a message
    static nMessageBase * TransformStatic( PROTOBUF const & message )
    {
        tASSERT( instance_ );

        // if there are multiple instances of descriptors of the same message class,
        // you can't pick one reliably.
        tASSERT( !instance_->multipleInstances_ );

        return instance_->Transform( message );
    }

    nProtoBufDescriptor( unsigned short identification, Handler * handler,
                   bool acceptEvenIfNotLoggedIn = false )
    : nProtoBufDescriptorBase( identification, PROTOBUF(), acceptEvenIfNotLoggedIn )
    , handler_( handler )
    {
#ifdef DEBUG
        multipleInstances_ = instance_;
#endif
        instance_ = this;
    }
    
    void Handle( PROTOBUF const & message, nSenderInfo const & sender ) const
    {
        tASSERT( handler_ );
        handler_( message, sender );
    }
    
    //! creates a message
    inline nProtoBufMessage< PROTOBUF > * CreateMessage() const
    {
        return tNEW(nProtoBufMessage< PROTOBUF >)( *this );
    }

private:
    //! instance of this descriptor
    static nProtoBufDescriptor * instance_;

    tJUST_CONTROLLED_PTR< nProtoBufMessage< PROTOBUF > > lastSent_;

#ifdef DEBUG
    bool multipleInstances_;
#endif
   
    //! function actually handling the incoming message
    Handler * handler_;
    
    //! creates a message
    virtual nProtoBufMessageBase * DoCreateMessage() const
    {
        return CreateMessage();
    }
};

// create a message from a pattern buffer
template< class PROTOBUF >
nMessageBase * nMessage::Transform( PROTOBUF const & message )
{
    return nProtoBufDescriptor< PROTOBUF >::TransformStatic( message );
}

//! instance of this descriptor
template< class PROTOBUF > 
nProtoBufDescriptor< PROTOBUF > * nProtoBufDescriptor< PROTOBUF >::instance_ = 0;

//! class for automatic conversion to 'basse' protobuffers
template< class SOURCE >
class nProtoBufBaseConverter
{
public:
    nProtoBufBaseConverter( SOURCE & source )
    : source_( source ){}

    operator SOURCE &()
    {
        return source_;
    }

    template< class DEST >
    operator DEST const &()
    {
        return Convert( source_.base() );
    }

    template< class DEST >
    operator DEST &()
    {
        return Convert( *source_.mutable_base() );
    }
private:
    template< class MEDIUM >
    nProtoBufBaseConverter< MEDIUM > Convert( MEDIUM & medium )
    {
        return nProtoBufBaseConverter< MEDIUM >( medium );
    }

    SOURCE & source_;
};

//! specialization of descriptor for each network object class
template< class OBJECT, class PROTOBUF >
class nOProtoBufDescriptor: public nOProtoBufDescriptorBase, public nProtoBufDescriptor< PROTOBUF >
{
public:
    nOProtoBufDescriptor( int identification )
    : nOProtoBufDescriptorBase()
    , nProtoBufDescriptor< PROTOBUF >( identification, &HandleIncoming, false )
    {
        this->SetStreamer( CreationStreamer() );
    }
private:
    // template message
    static const PROTOBUF prototype;

    static void HandleCreation( PROTOBUF const & message, nSenderInfo const & sender )
    {
        unsigned short id = GetObjectID( message );
        
        if( PreCheck( id, sender ) )
        {
            nNetObjectRegistrar registrar;
            tJUST_CONTROLLED_PTR< OBJECT > n=tNEW(OBJECT)( message, sender );
            n->InitAfterCreation();
            n->ReadSync( nProtoBufBaseConverter< PROTOBUF const >( message ), sender );
            n->Register( registrar );
            
            PostCheck( n, sender );
        }
    }

    static void HandleIncoming( PROTOBUF const & message, nSenderInfo const & sender )
    {
        unsigned short id = GetObjectID( message );
        OBJECT * object;
        nNetObject::IDToPointer( id, object );
        if ( object )
        {
            nProtoBufBaseConverter< PROTOBUF const > converter( message );
            if ( object->SyncIsNew( converter, sender ) )
            {
                object->ReadSync( converter, sender );
            }
        }
        else
        {
            HandleCreation( message, sender );
        }
    }

    //! reads sync, true work
    void DoReadSync( nNetObject & object, nProtoBufMessage< PROTOBUF > & envelope ) const
    {
        // cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );

        // read sync from message
        PROTOBUF & sync = envelope->AccessProtoBuf();

        // transfer sync to message
        ReadMessage( envelope, sync, envelope->AccessWorkProtoBuf() );

        // delegate to object
        nProtoBufBaseConverter< PROTOBUF const > converter( sync );
        nSenderInfo sender( envelope );
        if ( cast.SyncIsNew( converter, sender ) )
        {
            cast.ReadSync( converter, sender );
        }
    }

    //! creates a message
    nProtoBufMessage< PROTOBUF > * CreateMessage() const
    {
        return tNEW(nProtoBufMessage< PROTOBUF >)( *this );
    }

private:
    //! writes sync, true work
    nProtoBufMessageBase * DoWriteSync( nNetObject & object, bool create ) const
    {
        nProtoBufMessage< PROTOBUF > * message = this->CreateMessage();

        // cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );

        // write sync
        PROTOBUF & sync = message->AccessProtoBuf();
        cast.WriteSync( nProtoBufBaseConverter< PROTOBUF >( sync ), create );

        // set old streaming mode to sync mode
        if ( !create )
        {
            message->SetStreamer( SyncStreamer() );
        }

        return message;
    }

    //! reads a sync message
    virtual void DoReadSync( nNetObject & object, nStreamMessage & envelope, bool create ) const
    {
        // cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );

        // read sync
        PROTOBUF sync;
        StreamFrom( envelope, sync, create ? nProtoBufDescriptorBase::SECTION_First : nProtoBufDescriptorBase::SECTION_Second );
        cast.ReadSync( nProtoBufBaseConverter< PROTOBUF const >( sync ), nSenderInfo( envelope ) );
    }

    //! writes into an old sync message
    virtual void DoWriteSync( nNetObject & object, nStreamMessage & envelope, bool create ) const
    {
        // cast to correct type
        tASSERT( dynamic_cast< OBJECT * >( &object ) );
        OBJECT & cast = static_cast< OBJECT & >( object );

        // read sync
        PROTOBUF sync;
        cast.WriteSync( nProtoBufBaseConverter< PROTOBUF >( sync ), create );
        StreamTo( sync, envelope, create ? nProtoBufDescriptorBase::SECTION_First : nProtoBufDescriptorBase::SECTION_Second );
    }
    
    virtual nProtoBufMessageBase* DoCreateMessage() const
    {
        return CreateMessage();
    }
};

// template message
template< class OBJECT, class PROTOBUF >
const PROTOBUF nOProtoBufDescriptor< OBJECT, PROTOBUF >::prototype;

//! one cache for every protobuf descriptor
class nMessageCacheByDescriptor
{
    // trivial first implementation

    friend class nMessageCache;

    //! queue of cached messages
    typedef std::deque< tJUST_CONTROLLED_PTR< nProtoBufMessageBase > > CacheQueue;
    CacheQueue queue_;
};

//! message cache, used to compress and restore messages
//! based on previous messages.
class nMessageCache
{
public:
    //! clears the cache
    void Clear();

    //! adds a message to the cache
    void AddMessage( nProtoBufMessageBase * message, bool incoming, bool reallyAdd = true );

    //! adds a message to the cache
    void AddMessage( nMessageBase * message, bool incoming, bool reallyAdd = true );

    //! fill protobuf from cache. Returns true on success.
    bool UncompressProtoBuf( unsigned short cacheID, nProtoBuf & target );
    
    //! find suitable previous message and compresses
    //! the passed protobuf. Return value: the cache ID.
    unsigned short CompressProtoBuff( nProtoBuf const & source, nProtoBuf & target );

private:
    // the partial caches
    typedef std::map< google::protobuf::Descriptor const *, nMessageCacheByDescriptor > CacheMap;
    CacheMap parts;
};

#endif
