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

#define XXX

#include "nProtoBuf.h"
#include "tConsole.h"
#include "nNetObject.h"
#include "nBinary.h"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include "nNetObject.pb.h"

#include "nStreamMessage.h"
#include "tLocale.h"
#include "tConfiguration.h"

// #include <zlib.h>

using namespace google::protobuf;

#ifdef DEBUG
// print debug strings?
// #define DEBUG_STRINGS
#endif

#if GOOGLE_PROTOBUF_VERSION < 2000002
// cull first parameter from reflection functions
#define REFL_GET( function, message, field )        function( field )
#define REFL_SET( function, message, field, value ) function( field, value )
#define REFL_GET_REP( function, message, field, index )        function( field, index )
#define REFL_SET_REP( function, message, field, index, value ) function( field, index, value )
typedef nProtoBuf::Reflection Reflection;
#define REFLECTION_CONST Reflection
#else
#define REFL_GET( function, message, field )        function( message, field )
#define REFL_SET( function, message, field, value ) function( message, field, value )
#define REFL_GET_REP( function, message, field, index )        function( message, field, index )
#define REFL_SET_REP( function, message, field, index, value ) function( message, field, index, value )
#define REFLECTION_CONST Reflection const
#endif

void sn_PrintDebug( nProtoBuf const & buf )
{
    buf.PrintDebugString();
}

void sn_PrintDebug( nProtoBufMessageBase const & buf )
{
    buf.GetProtoBuf().PrintDebugString();
}

enum HeaderFlags
{
    FLAG_MessageID = 1,
    FLAG_CacheRef  = 2,
    FLAG_Unknown   = ~3
};

void nProtoBufHeader::Write( nBinaryWriter & writer )
{
    unsigned char flags = 
    ( messageID > 0 ? FLAG_MessageID : 0 ) |
    ( cacheRef  > 0 ? FLAG_CacheRef  : 0 );

    // write variable data
    writer.WriteByte( flags );
    if ( messageID > 0 )
    {
        writer.WriteShort( messageID );
    }
    if ( cacheRef > 0 )
    {
        if ( messageID > 0 )
        {
            // write a relative reference
            short relative = messageID - cacheRef;

            // yeah, writing the signed value as unsigned is
            // on purpose. Messages can't reference future messages.
            writer.WriteVarUInt( relative );
        }
        else
        {
            // write an absolute reference
            writer.WriteShort( cacheRef );
        }
    }

    writer.WriteVarUInt( len );
}

void nProtoBufHeader::Read( nBinaryReader & reader )
{
    // read variable data
    unsigned char flags = reader.ReadByte();
    if ( flags & FLAG_Unknown )
    {
        // unknown flags, reading will fail
        nReadError( true );
    }
    if( flags & FLAG_MessageID )
    {
        messageID = reader.ReadShort();
    }
    if( flags & FLAG_CacheRef )
    {
        if( flags & FLAG_MessageID )
        {
            short relative = reader.ReadVarUInt();
            cacheRef = messageID - relative;
        }
        else
        {
            cacheRef = reader.ReadShort();
        }
    }
    
    len = reader.ReadVarUInt();
}

nMessageStreamer::nMessageStreamer(){}

nMessageStreamer::nMessageStreamer( nProtoBufDescriptorBase & descriptor )
{
    descriptor.SetStreamer( this );
}

nMessageStreamer::~nMessageStreamer(){}

//! function responsible for turning message into old stream message
void nMessageStreamer::StreamFromProtoBuf( nProtoBufMessageBase const & source, nStreamMessage & target ) const
{
    // stream to message
    source.GetDescriptor().StreamTo( source.GetProtoBuf(), target, nProtoBufDescriptorBase::SECTION_First );
    
    // bend the descriptor
    target.BendDescriptorID( source.DescriptorID() & ~nProtoBufDescriptorBase::protoBufFlag );
}

//! function responsible for reading an old message
void nMessageStreamer::StreamToProtoBuf( nStreamMessage & source, nProtoBufMessageBase & target ) const
{
    // stream from message
    target.GetDescriptor().StreamFrom( source, target.AccessProtoBuf(), nProtoBufDescriptorBase::SECTION_First );
}

nProtoBufMessageBase::~nProtoBufMessageBase(){}

nProtoBufMessageBase::nProtoBufMessageBase( nProtoBufDescriptorBase const & descriptor, unsigned int messageID )
: nMessageBase( descriptor, messageID ), 
  streamer_( descriptor.GetStreamer() )
{}

nProtoBufMessageBase::nProtoBufMessageBase( nProtoBufDescriptorBase const & descriptor )
: nMessageBase( descriptor ), 
  streamer_( descriptor.GetStreamer() )
{}

bool sn_filterColorStrings = false;
static tConfItem<bool> sn_filterColorStringsConf("FILTER_COLOR_STRINGS",sn_filterColorStrings);
bool sn_filterDarkColorStrings = false;
static tConfItem<bool> sn_filterDarkColorStringsConf("FILTER_DARK_COLOR_STRINGS",sn_filterDarkColorStrings);

// filters a string
std::string sn_FilterString( std::string const & s, FieldDescriptor const * field )
{
    tColoredString filtered( s );

    if( field->name().rfind( "_raw" ) != std::string::npos )
    {
        // raw string, no filtering required
        return s;
    }

    // filter client string messages
    if ( sn_GetNetState() == nSERVER )
    {
        filtered.NetFilter();
        filtered.RemoveTrailingColor();
    }

    // filter color codes away
    if ( sn_filterColorStrings )
        filtered = tColoredString::RemoveColors( filtered, false );
    else if ( sn_filterDarkColorStrings )
        filtered = tColoredString::RemoveColors( filtered, true );

    return filtered;
}

// filter incoming messages for odd stuff. Strings, mostly.
void nProtoBufMessageBase::Filter( nProtoBuf & buf )
{
    // get reflection interface
    REFLECTION_CONST * r = buf.GetReflection();
    Descriptor const * descriptor  = buf.GetDescriptor();

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );
        tASSERT( field );
        
        if ( field->label() == FieldDescriptor::LABEL_REPEATED )
        {
            for( int j = r->REFL_GET( FieldSize, buf, field ) - 1; j >= 0; --j )
            {
                switch( field->cpp_type() )
                {
                case FieldDescriptor::CPPTYPE_STRING:
                {
                    std::string filter = r->REFL_GET_REP( GetRepeatedString, buf, field, j );
                    r->REFL_SET_REP( SetRepeatedString, &buf, field, j, sn_FilterString( filter, field ) );
                    break;
                }
                case FieldDescriptor::CPPTYPE_MESSAGE:
                    Filter( *r->REFL_GET_REP( MutableRepeatedMessage, &buf, field, j ) );
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            if ( !r->REFL_GET( HasField, buf, field ) )
            {
                continue;
            }

            switch( field->cpp_type() )
            {
            case FieldDescriptor::CPPTYPE_STRING:
            {
                std::string filter = r->REFL_GET( GetString, buf, field );
                r->REFL_SET( SetString, &buf, field, sn_FilterString( filter, field ) );
                break;
            }
            case FieldDescriptor::CPPTYPE_MESSAGE:
                Filter( *r->REFL_GET( MutableMessage, &buf, field ) );
                break;
            default:
                break;
            }
        }
    }
}

//! returns the descriptor
nProtoBufDescriptorBase const & nProtoBufMessageBase::GetDescriptor() const
{
    // we'd use covariant return types, but that only works in one direction,
    // and nProtoBufDescriptorBase has a covariant return that requires
    // this class definition to be complete.
    tASSERT( dynamic_cast< nProtoBufDescriptorBase const * >( &nMessageBase::GetDescriptor() ) );
    return static_cast< nProtoBufDescriptorBase const & >( nMessageBase::GetDescriptor() );
}

//! fills the receiving buffer with data
int nProtoBufMessageBase::OnWrite( WriteArguments & arguments ) const
{
    nProtoBuf const & fullProtoBuf = GetProtoBuf();

    // translate message for older clients
    {
        nProtoBufDescriptorBase const & descriptor = GetDescriptor();
        if ( descriptor.GetTranslator() )
        {
            tJUST_CONTROLLED_PTR< nMessageBase > translated = descriptor.GetTranslator()->
            Translate( fullProtoBuf, arguments.receiver_ );
            if ( translated )
            {
                // if a translation was required, send it.
                translated->BendMessageID( MessageID() );
                return translated->Write( arguments );
            }
        }
    }

#ifdef DEBUG
    fullProtoBuf.CheckInitialized();
#endif

    unsigned short descriptorID = DescriptorID();

    // does the receiver understand ProtoBufs?
    if ( sn_protoBuf.Supported( arguments.receiver_ ) )
    {
        nBinaryWriter writer( arguments.buffer_ );

        // yep. Stream them.
        // compress message, writing the cache ID
        nProtoBuf & workProtoBuf = AccessWorkProtoBuf();
        nMessageCache & cache = sn_Connections[ arguments.receiver_ ].messageCacheOut_;

        nProtoBufHeader header;
        header.cacheRef = cache.CompressProtoBuff( fullProtoBuf, workProtoBuf, arguments, cacheHint_ ); 

        // dump into plain stringstream
        std::stringstream rw;
        workProtoBuf.SerializeToOstream( &rw );

        // write header data
        header.len = rw.str().size();
        header.messageID = MessageID();
        header.Write( writer );

        // write stringstream to message
        unsigned int count = header.len;
        while( count > 0 )
        {
            tASSERT( rw.good() );
            writer.WriteByte( rw.get() );
            --count;
        }
    }
    else
    {
        // receiver does not understant ProtoBufs. Transform
        // to old style message.
        if ( !oldFormat_ )
        {
            // Transform to old style message.
            static nStreamDescriptor dummy( 0, NULL, NULL );
            oldFormat_ = tNEW(nStreamMessage)( dummy, MessageID(), SenderID() );

            tASSERT( streamer_ );
            streamer_->StreamFromProtoBuf( *this, *oldFormat_ );
        }

        // and copy the message contents over.
        static_cast< nMessageBase * >( oldFormat_ )->Write( arguments );

        // bend the descriptor
        descriptorID = oldFormat_->DescriptorID();
    }

    return descriptorID;
}

//! reads data from network buffer if the header was already read
void nProtoBufMessageBase::OnRead( unsigned char const * & buffer, unsigned char const * end )
{
    if( DescriptorID() & nProtoBufDescriptorBase::protoBufFlag )
    {
        // read header
        nBinaryReader reader( buffer, end );
        nProtoBufHeader header;
        header.Read( reader );

        messageIDBig_ = sn_ExpandMessageID( header.messageID, SenderID() );

        unsigned char const * payload = buffer;
        reader.Advance( header.len );
        
        // read the cache ID
        unsigned short cacheID = header.cacheRef;
        
        nProtoBuf & out = AccessProtoBuf();
        nProtoBuf & work = AccessWorkProtoBuf();
        
        // pre-fill
        nMessageCache & cache = sn_Connections[ SenderID() ].messageCacheIn_;
        bool inCache = cacheID && cache.UncompressProtoBuf( cacheID, out );
        // fill rest of fields
        
        if ( inCache )
        {
            work.ParsePartialFromArray( payload, header.len );
            work.DiscardUnknownFields();
            
#ifdef DEBUG_STRINGS
            con << "Base  : " << out.ShortDebugString() << "\n";
            con << "Diff  : " << work.ShortDebugString() << "\n";
#endif
            
            out.MergeFrom( work );
            
#ifdef DEBUG_STRINGS
            con << "Merged: " << out.ShortDebugString() << "\n";
#endif
        }
        else
        {
            // just read directly
            out.ParsePartialFromArray( payload, header.len );

            // another harmless leak source here
            tKnownExternalLeak l;
            out.DiscardUnknownFields();
        }
    }
    else
    {
        // old format, first read it that way, then transform
        if ( !oldFormat_ )
        {
            // read old format
            static nStreamDescriptor dummy( 0, NULL, NULL );
            oldFormat_ = tNEW(nStreamMessage)( dummy, MessageID(), SenderID() );
            static_cast< nMessageBase * >( oldFormat_ )->Read( buffer, end );
        }

        // transform
        tASSERT( streamer_ );
        streamer_->StreamToProtoBuf( *oldFormat_, *this );
        messageIDBig_ = oldFormat_->MessageIDBig();
    }
}

int nProtoBufMessageBase::Size() const
{
    return GetProtoBuf().ByteSize() + 5;
}

nMessageTranslatorBase::nMessageTranslatorBase(){}
nMessageTranslatorBase::~nMessageTranslatorBase(){}

//! constructor registering with the descriptor
nMessageTranslatorBase::nMessageTranslatorBase( nProtoBufDescriptorBase & descriptor )
{
    descriptor.SetTranslator( this );
}

//! convert current message format to format suitable for old client
//! @param source source protobuf
//! @param receiver the network ID of the receiver
nMessageBase * nMessageTranslatorBase::Translate( nProtoBuf const & source, int receiver ) const
{
    return NULL;
}

/*
Not defined here, but in nNetwork.cpp for access to the right static variables and macros.

nProtoBufDescriptorBase::nProtoBufDescriptorBase(unsigned short identification,
                                     const char * name, 
                                     bool acceptEvenIfNotLoggedIn )
*/

nMessageStreamer & nProtoBufDescriptorBase::GetDefaultStreamer()
{
    static nMessageStreamer streamer;
    return streamer;
}

std::string const & nProtoBufDescriptorBase::DetermineName( nProtoBuf const & prototype )
{
    // the protobuf initialization code allocates stuff without bothering to
    // deallocate it later. It's not a bad leak, the memory stays reachable,
    // but our memory manager needs to be told so so it doesn't annoy
    // with alarms.
    tKnownExternalLeak l;
    return prototype.GetDescriptor()->full_name();
}

nProtoBufDescriptorBase::DescriptorMap const & nProtoBufDescriptorBase::GetDescriptorsByName()
{
    return AccessDescriptorsByName();
}

nProtoBufDescriptorBase::DescriptorMap & nProtoBufDescriptorBase::AccessDescriptorsByName()
{
    static DescriptorMap descriptorsByName;
    return descriptorsByName;
}


//! dumb streaming from message, static version
void nProtoBufDescriptorBase::StreamFromStatic( nStreamMessage & in, nProtoBuf & out, StreamSections sections )
{
    // try to determine suitable descriptor
    nProtoBufDescriptorBase const * embedded = AccessDescriptorsByName()[ DetermineName( out ) ];
    if ( embedded )
    {
        // found it, delegate
        embedded->StreamFrom( in, out, sections );
    }
    else
    {
#ifdef DEBUG_X
        static bool warn = true;
        if ( warn )
        {
            tERR_WARN( "Unknown buffer format " << DetermineName( out ) << ", not sure if it'll get streamed correctly." );
            warn = false;
        }
#endif
        
        // fallback to default
        StreamFromDefault( in, out, sections );
    }
}

//! dumb streaming to message, static version
void nProtoBufDescriptorBase::StreamToStatic( nProtoBuf const & in, nStreamMessage & out, StreamSections sections )
{
    // try to determine suitable descriptor
    nProtoBufDescriptorBase const * embedded = AccessDescriptorsByName()[ DetermineName( in ) ];
    if ( embedded )
    {
        // found it, delegate
        embedded->StreamTo( in, out, sections );
    }
    else
    {
#ifdef DEBUG_X
        static bool warn = true;
        if ( warn )
        {
            tERR_WARN( "Unknown buffer format, not sure if it'll get streamed correctly." );
            warn = false;
        }
#endif
        
        // fallback to default
        StreamToDefault( in, out, sections );
    }
}

//! dumb streaming from message, static version
void nProtoBufDescriptorBase::StreamFromDefault( nStreamMessage & in, nProtoBuf & out, StreamSections sections )
{
    // get reflection interface
    REFLECTION_CONST * reflection = out.GetReflection();
    Descriptor const * descriptor = out.GetDescriptor();

    // flags for the current section
    int currentSectionFlags = 1;

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        // out.PrintDebugString();

        FieldDescriptor const * field = descriptor->field( i );
        tASSERT( field );

        if ( in.End() )
        {
            break;
        }

        if ( FieldDescriptor::kLastReservedNumber < field->number() )
        {
            // end marker
            currentSectionFlags <<= 2;
            continue;
        }

        // skip over fields if section doesn't match
        if ( ( 0 == ( sections & currentSectionFlags ) ) && ( ( sections != SECTION_ID && field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE ) || i != 0 ) )
        {
            if( currentSectionFlags > sections )
            {
                break;
            }

            continue;
        }

        if ( field->label() == FieldDescriptor::LABEL_REPEATED )
        {
            if ( i != 0 )
            {
                // This case doesn't happen too often. Actually, just once:
                // the gNetPlayerWall sync. All quirks here are just to get that
                // one case right.

                // read field size
                unsigned short size;
                in.Read( size );
                
                // clear the array
                reflection->REFL_GET( ClearField, &out, field );
                
                switch( field->cpp_type() )
                {
                case FieldDescriptor::CPPTYPE_MESSAGE:
                {
                    // read field in reverse order
                    for( int j = size-1; j >= 0; --j )
                    {
                        reflection->REFL_GET( AddMessage, &out, field );
                    }
                    
                    for( int j = size-1; j >= 0; --j )
                    {
                        StreamFromStatic( in, *reflection->REFL_GET_REP( MutableRepeatedMessage, &out, field, j ), SECTION_First );
                    }
                }
                break;
                default:
                    break;
                }

                continue;
            }
            else
            {
                // the other repeated arrays are just fill-the-message types, where no
                // lenght is streamed and the array ends when the message ends.
                // clear the array
                reflection->REFL_GET( ClearField, &out, field );
                while( !in.End() )
                {
                    switch( field->cpp_type() )
                    {
                    case FieldDescriptor::CPPTYPE_UINT32:
                    {
                        unsigned short value;
                        in.Read( value );
                        reflection->REFL_SET( AddUInt32, &out, field, value );
                        break;
                    }
                    case FieldDescriptor::CPPTYPE_MESSAGE:
                    {
                        nProtoBuf * message = reflection->REFL_GET( AddMessage, &out, field );
                        StreamFromStatic( in, *message, SECTION_First );
                        break;
                    }
                    default:
                        break;
                    }
                }
                
                break;
            }
            
        }

        switch( field->cpp_type() )
        {
        case FieldDescriptor::CPPTYPE_INT32:
        {
            int32 value;
            in >> value;
            reflection->REFL_SET( SetInt32, &out, field, value );
        }
        break;
        case FieldDescriptor::CPPTYPE_UINT32:
        {
            unsigned short value;
            in.Read( value );
            reflection->REFL_SET( SetUInt32, &out, field, value );
        }
        break;
        case FieldDescriptor::CPPTYPE_STRING:
        {
            tString value;
            in.ReadRaw( value );
            
            reflection->REFL_SET( SetString, &out, field, st_Latin1ToUTF8( value ) );
        }
        break;
        case FieldDescriptor::CPPTYPE_FLOAT:
        {
            REAL value;
            in >> value;
            reflection->REFL_SET( SetFloat, &out, field, value );
        }
        break;
        case FieldDescriptor::CPPTYPE_BOOL:
        {
            bool value;
            in >> value;
            reflection->REFL_SET( SetBool, &out, field, value );
        }
        break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            StreamFromStatic( in, *reflection->REFL_GET( MutableMessage, &out, field ), i ? SECTION_First : sections );
            break;

        // explicitly unsupported:
        case FieldDescriptor::CPPTYPE_ENUM:
        case FieldDescriptor::CPPTYPE_INT64:
        case FieldDescriptor::CPPTYPE_UINT64:
        case FieldDescriptor::CPPTYPE_DOUBLE:
        default:
            tERR_ERROR( "Unsupported field type." );
            break;
        }
    }
}

//! dumb streaming to message, static version
void nProtoBufDescriptorBase::StreamToDefault( nProtoBuf const & in, nStreamMessage & out, StreamSections sections )
{
    // get reflection interface
    const Reflection * reflection = in.GetReflection();
    const Descriptor * descriptor = in.GetDescriptor();

    // flags for the current section
    int currentSectionFlags = 1;

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );

        if ( FieldDescriptor::kLastReservedNumber < field->number() )
        {
            // end marker
            currentSectionFlags <<= 2;
            continue;
        }

        // skip over fields if section doesn't match
        if ( ( 0 == ( sections & currentSectionFlags ) ) && ( ( sections != SECTION_ID && field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE ) || i != 0 ) )
        {
            if( currentSectionFlags > sections )
            {
                break;
            }

            continue;
        }

        if ( field->label() == FieldDescriptor::LABEL_REPEATED )
        {
            if ( i != 0 )
            {
                // con << in.ShortDebugString() << "\n";
                
                // This case doesn't happen too often. Actually, just once:
                // the gNetPlayerWall sync. All quirks here are just to get that
                // one case right.
                
                // read field size
                unsigned short size = reflection->REFL_GET( FieldSize, in, field );
                if ( size == 0 )
                {
                    continue;
                }
                
                out.Write( size );
                
                // clear the array
                
                switch( field->cpp_type() )
                {
                case FieldDescriptor::CPPTYPE_MESSAGE:
                {
                    // write fields in reverse order
                    for( int j = size-1; j >= 0; --j )
                    {
                        StreamToStatic( reflection->REFL_GET_REP( GetRepeatedMessage, in, field, j ), out, SECTION_First );
                    }
                }
                break;
                default:
                    break;
                }

                continue;
            }
            else
            {
                unsigned short size = reflection->REFL_GET( FieldSize, in, field );
                for( int j = 0; j < size; ++j )
                {
                    switch( field->cpp_type() )
                    {
                    case FieldDescriptor::CPPTYPE_UINT32:
                    {
                        out.Write( reflection->REFL_GET_REP( GetRepeatedUInt32, in, field, j ) );
                        break;
                    }
                    case FieldDescriptor::CPPTYPE_MESSAGE:
                    {
                        nProtoBuf const & message = reflection->REFL_GET_REP( GetRepeatedMessage, in, field, j );
                        StreamToStatic( message, out, SECTION_First );
                        break;
                    }
                    default:
                        break;
                    }
                }

                break;
            }

        }

        switch( field->cpp_type() )
        {
        case FieldDescriptor::CPPTYPE_INT32:
            out << reflection->REFL_GET( GetInt32, in, field );
            break;
        case FieldDescriptor::CPPTYPE_UINT32:
            out.Write( reflection->REFL_GET( GetUInt32, in, field ) );
            break;
        case FieldDescriptor::CPPTYPE_STRING:
            out << tString( reflection->REFL_GET( GetString, in, field ) );
            break;
        case FieldDescriptor::CPPTYPE_FLOAT:
            out << reflection->REFL_GET( GetFloat, in, field );
            break;
        case FieldDescriptor::CPPTYPE_BOOL:
            out << reflection->REFL_GET( GetBool, in, field );
            break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            StreamToStatic( reflection->REFL_GET( GetMessage, in, field ), out, i ? SECTION_First : sections );
            break;

        // explicitly unsupported:
        case FieldDescriptor::CPPTYPE_ENUM:
        case FieldDescriptor::CPPTYPE_INT64:
        case FieldDescriptor::CPPTYPE_UINT64:
        case FieldDescriptor::CPPTYPE_DOUBLE:
        default:
            tERR_ERROR( "Unsupported field type." );
            break;
        }
    }
}   

//! compares two messages, filling in total size and difference.
//! @param a first message
//! @param b second message
//! @param total total maximal size of message (must be zeroed before call)
//! @param total difference of messages, between 0 and total (must be zeroed before call)
//! @param removed is set to true if a has an element that is missing from b
void nProtoBufDescriptorBase::EstimateMessageDifference( nProtoBuf const & a,
                                                   nProtoBuf const & b,
                                                   int & total,
                                                   int & difference,
                                                   bool & removed )
{
    // get reflection interface
    const Reflection * ra = a.GetReflection();
    Descriptor const * descriptor  = a.GetDescriptor();
    const Reflection * rb = b.GetReflection();
    tASSERT( descriptor == b.GetDescriptor() );

#ifdef DEBUG
    // tString da = a.ShortDebugString();
    // tString db = b.ShortDebugString();
#endif    

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );
        tASSERT( field );

        int weight = 0;
        bool differ = false;

        if ( field->label() == FieldDescriptor::LABEL_REPEATED )
        {
            continue;
        }

        switch( field->cpp_type() )
        {
#define COMPARE( getter, w ) weight = w; differ = ( ra->REFL_GET( getter, a, field ) != rb->REFL_GET( getter, b, field ) )
        case FieldDescriptor::CPPTYPE_INT32:
            COMPARE( GetInt32, 2 );
            break;
        case FieldDescriptor::CPPTYPE_UINT32:
            COMPARE( GetUInt32, 2 );
            break;
        case FieldDescriptor::CPPTYPE_STRING:
            COMPARE( GetString, 8 );
            break;
        case FieldDescriptor::CPPTYPE_FLOAT:
            COMPARE( GetFloat, 4 );
            break;
        case FieldDescriptor::CPPTYPE_BOOL:
            COMPARE( GetBool, 1 );
            break;
        case FieldDescriptor::CPPTYPE_ENUM:
            COMPARE( GetEnum, 2 );
            break;
    case FieldDescriptor::CPPTYPE_INT64:
            COMPARE( GetInt64, 4 );
            break;
        case FieldDescriptor::CPPTYPE_UINT64:
            COMPARE( GetUInt64, 4 );
            break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
            COMPARE( GetDouble, 8 );
            break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            EstimateMessageDifference( ra->REFL_GET( GetMessage, a, field ), rb->REFL_GET( GetMessage, b, field ), total, difference, removed );
            break;
        default:
            tERR_ERROR( "Unsupported field type." );
            break;
#undef COMPARE
        }
    
        // accounting
        total += weight;
        if ( differ || ra->REFL_GET( HasField, a, field ) != rb->REFL_GET( HasField, b, field ) )
        {
            difference += weight;
        }

        // check for missing fields
        if ( ra->REFL_GET( HasField, a, field ) && ! rb->REFL_GET( HasField, b, field ) )
        {
            removed = true;
        }
    }
}

//! calculates the difference between two messages
//! @param base base message
//! @param derived second message
//! @param diff target message for difference. All the fields where base and derived differ are set here, with the value taken from derived.
//! @param copy if true, the first operation is a copy from derived to diff. If false, only elements equal in both derived and base are discarded from diff.
void nProtoBufDescriptorBase::DiffMessages( nProtoBuf const & base,
                                      nProtoBuf const & derived,
                                      nProtoBuf & diff,
                                      bool copy )
{
    // get reflection interface
    Reflection const * r_base     = base.GetReflection();
    Descriptor const * descriptor = base.GetDescriptor();
    Reflection const * r_derived  = derived.GetReflection();
    REFLECTION_CONST *  r_diff     = diff.GetReflection();
    tASSERT( descriptor == derived.GetDescriptor() );
    tASSERT( descriptor == diff.GetDescriptor() );

    // make copy
    if ( copy )
    {
        diff.CopyFrom( derived );
    }

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );
        tASSERT( field );

        if ( field->label() == FieldDescriptor::LABEL_REPEATED )
        {
            continue;
        }

        if ( !r_base->REFL_GET( HasField, base, field ) )
        {
            continue;
        }
        
        bool differ = false;
        switch( field->cpp_type() )
        {
#define COMPARE( getter ) differ = ( r_base->REFL_GET( getter, base, field ) != r_derived->REFL_GET( getter, derived, field ) )
        case FieldDescriptor::CPPTYPE_INT32:
            COMPARE( GetInt32 );
            break;
        case FieldDescriptor::CPPTYPE_UINT32:
            COMPARE( GetUInt32 );
            break;
        case FieldDescriptor::CPPTYPE_STRING:
            COMPARE( GetString );
            break;
        case FieldDescriptor::CPPTYPE_FLOAT:
            COMPARE( GetFloat );
            break;
        case FieldDescriptor::CPPTYPE_BOOL:
            COMPARE( GetBool );
            break;
        case FieldDescriptor::CPPTYPE_ENUM:
            COMPARE( GetEnum );
            break;
    case FieldDescriptor::CPPTYPE_INT64:
            COMPARE( GetInt64 );
            break;
        case FieldDescriptor::CPPTYPE_UINT64:
            COMPARE( GetUInt64 );
            break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
            COMPARE( GetDouble );
            break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            differ = true;
            DiffMessages( r_base->REFL_GET( GetMessage, base, field ), r_derived->REFL_GET( GetMessage, derived, field ), *r_diff->REFL_GET( MutableMessage, &diff, field ), false );
            break;
        default:
            tERR_ERROR( "Unsupported field type." );
            break;
#undef COMPARE
        }
    
        if ( !differ )
        {
            // clear the field
            r_diff->REFL_GET( ClearField, &diff, field );
        }
    }
}

//! @param message the message to rid of all repeated fields
void nProtoBufDescriptorBase::ClearRepeated( nProtoBuf & message )
{
    // get reflection interface
    Descriptor const * descriptor = message.GetDescriptor();
    REFLECTION_CONST * reflection = message.GetReflection();

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );
        tASSERT( field );

        if ( field->label() == FieldDescriptor::LABEL_REPEATED )
        {
            // clear the field
            reflection->REFL_GET( ClearField, &message, field );
            continue;
        }

        switch( field->cpp_type() )
        {
        case FieldDescriptor::CPPTYPE_MESSAGE:
            ClearRepeated( *reflection->REFL_GET( MutableMessage, &message, field ) );
            break;
        default:
            break;
        }
    }
}

//! dumb streaming to message
void nProtoBufDescriptorBase::DoStreamTo( nProtoBuf const & in, nStreamMessage & out, StreamSections sections ) const
{
    StreamToDefault( in, out, sections );
}

//! dumb streaming from message
void nProtoBufDescriptorBase::DoStreamFrom( nStreamMessage & in, nProtoBuf & out, StreamSections sections ) const
{
    StreamFromDefault( in, out, sections );
}

/*
// read/write operators for protocol buffers
nStreamMessage& operator >> ( nStreamMessage& m, nProtoBuf & buffer )
{

    return m;
}

nStreamMessage& operator << ( nStreamMessage& m, nProtoBuf const & buffer )
{
    tASSERT(0);

    // dump into plain stringstream
    std::stringstream rw;
    buffer.SerializeToOstream( &rw );

    // write stringstream to message
    while( !rw.eof() )
    {
        unsigned short value = ((unsigned char)rw.get()) << 8;
        if ( !rw.eof() )
        {
            value += (unsigned char)rw.get();
        }
        m.Write( value );
    }

    // mark message as final
    m.Finalize();
   
    return m;
}
*/

nNetObjectDescriptorBase::~nNetObjectDescriptorBase()
{}

Network::NetObjectSync const & nNetObjectDescriptorBase::GetNetObjectSync ( Network::NetObjectSync const & message )
{
    return message;
}

unsigned int nNetObjectDescriptorBase::GetObjectID ( Network::NetObjectSync const & message )
{
    if( !message.has_object_id() )
    {
        nReadError( true );
    }
    return message.object_id();
}

//! checks to run before creating a new object
//! @return true if all is OK
bool nNetObjectDescriptorBase::PreCheck( Network::NetObjectSync const & sync, nSenderInfo sender )
{
    unsigned short id = GetObjectID( sync );
    nNetObject::ClearDeleted(id);

    // reject sync messages for nonexistent objects
    if( !sync.has_owner_id() && !bool(sn_netObjects[id]) )
    {
        return false;
    }

    if (sn_netObjectsOwner[id]!=sender.SenderID() || bool(sn_netObjects[id]))
    {
#ifdef DEBUG
        st_Breakpoint();
        if (!sn_netObjects[id])
        {
            con << "Netobject " << id << " is already reserved!\n";
        }
        else
        {
            con << "Netobject " << id << " is already defined!\n";
        }
#endif
        if (sn_netObjectsOwner[id]!=sender.SenderID())
        {
            Cheater( sender.SenderID() );
            nReadError();
        }

        return false;
    }

    // all OK
    return true;
}

//! checks to run after creating a new object
void nNetObjectDescriptorBase::PostCheck( nNetObject * object, nSenderInfo sender )
{
#ifdef DEBUG
    /*
      tString str;
      n->PrintName( str );
      con << "Received object " << str << "\n";
    */
#endif
            
    if ( sn_GetNetState()==nSERVER && !object->AcceptClientSync() )
    {
        object->Release();
        Cheater( sender.SenderID() ); // cheater!
    }
    else if ( static_cast< nNetObject* >( sn_netObjects[ object->ID() ] ) != object )
    {
        // object was unable to be registered
        object->Release(); // silently delete it.
    }
}

//! clears the cache
void nMessageCache::Clear()
{
    parts.clear();
}

// size of the compression cache in messages
static int sn_messageCacheSize = 1000;

// effort taken to find an optimal message in the cache
static REAL sn_messageCacheEffort = 1.0;

// returns the ID of the last message we should keep in the cache
static unsigned long sn_GetLastCachedMessageID( bool incoming, unsigned long lastKnownID )
{
    const int backlog = sn_messageCacheSize;
    int back = backlog;
    if ( incoming )
    {
        back = 100 + ( back << 1 );
    }
    int max = 0xffff;
    if ( !incoming )
    {
        max /= 2;
    }
    if ( back > max )
    {
        back = max;
    }

    return lastKnownID - back;
}

// returns the ID of the last message we should keep in the outgoing cache
static unsigned long sn_GetLastCachedMessageIDOutgoing()
{
    return sn_GetLastCachedMessageID( false, nMessageBase::CurrentMessageIDBig() );
}

//! adds a message to the cache
//! @param message the message to add
//! @param incoming true for incoming messages, keep an extra long backlog then
//! @param reallyAdd true if message should be added. Otherwise, we just purge old messages.
void nMessageCache::AddMessage( nProtoBufMessageBase * message, bool incoming, bool reallyAdd )
{
    // no cache, no compression.
    if( sn_messageCacheSize == 0 )
    {
        return;
    }

    // fetch message filler, ignoring invalid ones
    if ( !message || message->MessageID() == 0 )
    {
        return;
    }

#ifdef DEBUG_STRINGS
    if ( !reallyAdd )
    {
        con << "NOT ";
    }
    con << "Adding message " << message->MessageID() << " to cache.\n";
#endif    

    // get the cache belonging to this descriptor
    Descriptor const * descriptor = message->GetProtoBuf().GetDescriptor();
    nMessageCacheByDescriptor & cache = parts[ descriptor ];

    // and push the message into the cache
    if ( reallyAdd )
    {
        cache.queue_.push_back( message );
    }

    // purge old messages
    unsigned long back = sn_GetLastCachedMessageID( incoming, message->MessageID() );
    while ( cache.queue_.size() > 0 && long( back - cache.queue_.front()->MessageID() ) > 0 )
    {
        cache.queue_.pop_front();
    }
}

//! adds a message to the cache
//! @param message the message to add
//! @param incoming true for incoming messages, keep an extra long backlog then
//! @param reallyAdd true if message should be added. Otherwise, we just purge old messages.
void nMessageCache::AddMessage( nMessageBase * message, bool incoming, bool reallyAdd )
{
    // no cache, no compression.
    if( sn_messageCacheSize == 0 )
    {
        return;
    }

    nProtoBufMessageBase * message2 = dynamic_cast< nProtoBufMessageBase * >( message );
    if( message2 )
    {
        AddMessage( message2, incoming, reallyAdd );
    }
}

//! fill protobuf from cache
bool nMessageCache::UncompressProtoBuf( unsigned short cacheID, nProtoBuf & target )
{
    // clear target
    target.Clear();

    // do nothing if told so
    if ( cacheID == 0 )
    {
        return false;
    }

    // get the cache belonging to this descriptor
    Descriptor const * descriptor = target.GetDescriptor();
    nMessageCacheByDescriptor & cache = parts[ descriptor ];

    // find the cacheID
    typedef nMessageCacheByDescriptor::CacheQueue CacheQueue;
    for( CacheQueue::reverse_iterator i = cache.queue_.rbegin();
         i != cache.queue_.rend(); ++i )
    {
        nProtoBufMessageBase * message = *i;
        if ( ( message->MessageID() & 0xffff ) == cacheID )
        {
            // found it, copy over
            target.CopyFrom( message->GetProtoBuf() );
            
            nProtoBufDescriptorBase::ClearRepeated( target );

            return true;
        }
    }

#ifdef DEBUG
    con << "Cache miss : " << cacheID << "\n";
#endif
    nReadError( false );

    return false;
}

#ifdef DEBUG_X
class Stat
{
public:
    int saved, wasted, total;
    
    Stat():saved(0), wasted(0), total(0){}
    ~Stat()
    {
        con << "Total zlib: " << total << "\n";
        con << "Total zlib saving: " << saved << "\n";
        con << "Total zlib waste: " << wasted << "\n";
    }

    void Add( int total, int saving )
    {
        this->total += total;
        if ( saving > 0 )
        {
            saved += saving;
        }
        else
        {
            wasted -= saving;
        }
    }
};
#endif

static void sn_CheckMessage
(
    nProtoBuf const & source,       // message to compress
    nProtoBufMessageBase * message, // the message to check
    int & size,                     // total size
    int & bestDifference,           // best (lowest) difference so far
    nProtoBufMessageBase * & best,  // best message so far
    unsigned long lastMessageID     // last cached message ID
    )
{
    nProtoBuf const & cached = message->GetProtoBuf();

    if ( !message && message->MessageIDBig() == 0 )
    {
        return;
    }

    // ignore messages that are older than the first message in the queue.
    // they may already be expired on the receiver side.
    if ( long( lastMessageID - message->MessageIDBig() ) > 0 )
    {
        int x;
        x = 0;
        return;
    }
    
    // the current difference
    int difference = 0;
    size = 0;
    
    // whether there were removed fields
    bool removed = false;
    
    nProtoBufDescriptorBase::
    EstimateMessageDifference( cached, source,
                               size, difference, removed );
    if ( !removed && ( !best || difference < bestDifference ) )
    {
        // best message so far
        best = message;
        bestDifference = difference;
    }
}

//! find suitable previous message and compresses
//! the passed protobuf. Return value: the cache ID.
unsigned short nMessageCache::CompressProtoBuff( nProtoBuf const & source, nProtoBuf & target, nMessageBase::WriteArguments const & arguments, nProtoBufMessageBase * hint )
{
    // no cache or peer request not to compress, no compression.
    nConnectionInfo const & receiver = sn_Connections[arguments.receiver_];
    if( sn_messageCacheSize == 0 || ! ( receiver.zipCompression || receiver.diffCompression ) )
    {
        target.CopyFrom( source );
        return 0;
    }

    // get the cache belonging to this descriptor
    Descriptor const * descriptor = target.GetDescriptor();
    nMessageCacheByDescriptor & cache = parts[ descriptor ];

    // try to find the best matching message in the queue
    nProtoBufMessageBase * best = 0;

    // fetch the last cached message ID
    unsigned long lastMessageID = sn_GetLastCachedMessageIDOutgoing();

    // total size, maximum size to save
    int size = 0;
    
    // best difference achieved so far
    int bestDifference = 0;
    
    // attempts so far
    int count = 0;

    // count when last best message was found
    int bestCount = 0;

    // check the cache hint
    if( hint )
    {
        sn_CheckMessage( source, hint, size, bestDifference, best, lastMessageID );
    }

    // find the cacheID from the cache
    typedef nMessageCacheByDescriptor::CacheQueue CacheQueue;
    for( CacheQueue::reverse_iterator i = cache.queue_.rbegin();
         i != cache.queue_.rend(); ++i )
    {
        // check whether it pays to look on
        if ( bestDifference * count >= sn_messageCacheEffort * bestCount * size )
        {
            break;
        }

        sn_CheckMessage( source, *i, size, bestDifference, best, lastMessageID );

        ++count;
    }
    
    if ( best )
    {
        // found a good previous message. yay!
        nProtoBuf const & cached = best->GetProtoBuf();

        // calculate diff message
        nProtoBufDescriptorBase::
        DiffMessages( cached, source, target );

#ifdef DEBUG_STRINGS_X
        con << "Size = " << size << ", diff = " << bestDifference << "\n";
        con << lastMessageID << ", " << best->MessageIDBig() << "\n";
        con << "Base  : " << cached.ShortDebugString() << "\n";
        con << "Source: " << source.ShortDebugString() << "\n";
        con << "Diff  : " << target.ShortDebugString() << "\n";

#ifdef DEBUG_X
        // test zlib
        {
            #define SIZE 10000
            Bytef basebuf[SIZE], diffbuf[SIZE], fullbuf[SIZE];

            Bytef buf[SIZE];

            cached.SerializeToArray(&basebuf,SIZE);
            target.SerializeToArray(&diffbuf,SIZE);
            source.SerializeToArray(&fullbuf,SIZE);

            z_stream stream;
            memset( &stream, 0, sizeof( stream ) );

            stream.next_in  = basebuf;
            stream.avail_in = cached.GetCachedSize();
            
            stream.next_out = buf;
            stream.avail_out = SIZE;

            stream.total_in = 0;
            stream.total_out = 0;
            stream.msg = NULL;
            stream.state = NULL;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;

            deflateInit( &stream, 9 );

            // flush 1
            deflate( &stream, Z_SYNC_FLUSH );
            tASSERT( stream.avail_in == 0 );
            int base = stream.total_out;

            // update
            stream.next_in  = fullbuf;
            // stream.avail_in = source.GetCachedSize();
            stream.avail_in = 1;

            // flush 2
            deflate( &stream, Z_SYNC_FLUSH );
            tASSERT( stream.avail_in == 0 );
            deflateEnd( &stream );
            tASSERT( stream.avail_in == 0 );
            int second = stream.total_out;

            int reduced = second - base;

            static Stat stat;
            stat.Add( target.GetCachedSize(), target.GetCachedSize() - reduced );
            
            std::cout.flush();

            con << "zlib: " 
                << cached.GetCachedSize() << " to " 
                << target.GetCachedSize() << " to " 
                << reduced << "\n";

            std::cout.flush();
        }
#endif

#endif

        // and return the message ID
        return static_cast< unsigned short >( best->MessageID() );
    }
    else
    {
        // no message found, return full protobuf
        target.CopyFrom( source );
        return 0;
    }
}

#include "nConfig.h"

static nSettingItem< int > sn_messageCacheSizeConf( "MESSAGE_CACHE_SIZE", sn_messageCacheSize );
static tSettingItem< REAL > sn_messageCacheEffortConf( "MESSAGE_CACHE_EFFORT", sn_messageCacheEffort );

