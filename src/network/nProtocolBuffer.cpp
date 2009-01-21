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

#include "nProtocolBuffer.h"
#include "tConsole.h"
#include "nNetObject.h"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include "nNetObject.pb.h"

using namespace google::protobuf;

#if GOOGLE_PROTOBUF_VERSION < 2000003
// cull first parameter from reflection functions
#define REFL_GET( function, message, field )        function( field )
#define REFL_SET( function, message, field, value ) function( field, value )
typedef Message::Reflection Reflection;
#define REFLECTION_CONST Reflection
#else
#define REFL_GET( function, message, field )        function( message, field )
#define REFL_SET( function, message, field, value ) function( message, field, value )
#define REFLECTION_CONST Reflection const
#endif

/*
Not defined here, but in nNetwork.cpp for access to the right static variables and macros.

nPBDescriptorBase::nPBDescriptorBase(unsigned short identification,
                                     const char * name, 
                                     bool acceptEvenIfNotLoggedIn )
*/

std::string const & nPBDescriptorBase::DetermineName( Message const & prototype )
{
    return prototype.GetDescriptor()->full_name();
}

nPBDescriptorBase::DescriptorMap const & nPBDescriptorBase::GetDescriptorsByName()
{
    return descriptorsByName;
}

nPBDescriptorBase::DescriptorMap nPBDescriptorBase::descriptorsByName;
//! dumb streaming from message, static version
void nPBDescriptorBase::StreamFromStatic( nMessage & in, Message & out  )
{
    // try to determine suitable descriptor
    nPBDescriptorBase const * embedded = descriptorsByName[ DetermineName( out ) ];
    if ( embedded )
    {
        // found it, delegate
        embedded->StreamFrom( in, out );
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
        StreamFromDefault( in, out );
    }
}

//! dumb streaming to message, static version
void nPBDescriptorBase::StreamToStatic( Message const & in, nMessage & out )
{
    // try to determine suitable descriptor
    nPBDescriptorBase const * embedded = descriptorsByName[ DetermineName( in ) ];
    if ( embedded )
    {
        // found it, delegate
        embedded->StreamTo( in, out );
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
        StreamToDefault( in, out );
    }
}

//! dumb streaming from message, static version
void nPBDescriptorBase::StreamFromDefault( nMessage & in, Message & out  )
{
    // get reflection interface
    REFLECTION_CONST * reflection = out.GetReflection();
    Descriptor const * descriptor = out.GetDescriptor();

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );
        tASSERT( field );

        if ( in.End() )
        {
            break;
        }

        if ( FieldDescriptor::kLastReservedNumber < field->number() )
        {
            // end marker
            break;
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
            in >> value;
            reflection->REFL_SET( SetString, &out, field, value );
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
            StreamFromStatic( in, *reflection->REFL_GET( MutableMessage, &out, field ) );
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
void nPBDescriptorBase::StreamToDefault( Message const & in, nMessage & out )
{
    // get reflection interface
    const Reflection * reflection = in.GetReflection();
    const Descriptor * descriptor = in.GetDescriptor();

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );

        if ( FieldDescriptor::kLastReservedNumber < field->number() )
        {
            // end marker
            break;
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
            out << reflection->REFL_GET( GetString, in, field );
            break;
        case FieldDescriptor::CPPTYPE_FLOAT:
            out << reflection->REFL_GET( GetFloat, in, field );
            break;
        case FieldDescriptor::CPPTYPE_BOOL:
            out << reflection->REFL_GET( GetBool, in, field );
            break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            StreamToStatic( reflection->REFL_GET( GetMessage, in, field ), out );
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

//! selective writing to message, either embedded or transformed
void nPBDescriptorBase::WriteMessage( Message const & in, nMessage & out ) const
{
#ifdef DEBUG
    in.CheckInitialized();
#endif

#ifdef DEBUG
    {
        // stats
        nMessage a( *this ), b( *this );
        a << in;
        StreamTo( in, b );
        con << "Old size: "  << b.DataLen()*2
            << ",new size: " << a.DataLen()*2
            << ".\n";
    }
#endif
    

    if ( sn_protocolBuffers.Supported() )
    {
        tASSERT( out.descriptor & protoBufFlag );

        // write the message in its native format
        out << in;
    }
    else
    {
        // strip protocol buffer flag from message descriptor
        out.descriptor &= ~protoBufFlag;

        // transform the message to our legacy format
        StreamTo( in, out );
    }
}

//! selective reading message, either embedded or transformed
void nPBDescriptorBase::ReadMessage( nMessage & in, Message & out  ) const
{
    if ( in.descriptor & protoBufFlag )
    {
        // message is a proper protocol buffer message
        in >> out;
    }
    else
    {
        // transform from old format
        StreamFrom( in, out );
    }
}

//! compares two messages, filling in total size and difference.
//! @param a first message
//! @param b second message
//! @param total total maximal size of message (must be zeroed before call)
//! @param total difference of messages, between 0 and total (must be zeroed before call)
void nPBDescriptorBase::EstimateMessageDifference( Message const & a,
                                                    Message const & b,
                                                    int & total,
                                                    int & difference )
{
    tASSERT(0);
}

//! calculates the difference between two messages
//! @param base base message
//! @param derived second message
//! @param diff target message for difference. All the fields where base and derived differ are set here, with the value taken from derived.
void nPBDescriptorBase::DiffMessages( Message const & base,
                                      Message const & derived,
                                      Message & diff )
{
    tASSERT(0);
}

//! dumb streaming to message
void nPBDescriptorBase::DoStreamTo( Message const & in, nMessage & out ) const
{
    StreamToDefault( in, out );
}

//! dumb streaming from message
void nPBDescriptorBase::DoStreamFrom( nMessage & in, Message & out ) const
{
    StreamFromDefault( in, out );
}

// read/write operators for protocol buffers
nMessage& operator >> ( nMessage& m, Message & buffer )
{
    // first, read into a string. protobufs ALWAYS take the whole rest of a
    // message. Luckily, they don't mind a single appended 0 byte.
    std::stringstream rw;
    while( !m.End() )
    {
        unsigned short value;
        m.Read( value );
        rw.put( value >> 8 );
        rw.put( value & 0xff );
    }

    // then, read the string into the buffer
    buffer.ParseFromIstream( &rw );

    // TODO: optimize. The stream copy should not be required, the data is
    // already in memory. But first, make it so that the byte order in memory
    // is already the network byte order, right now, it's the machine's order.

    // also, maybe postpone the actual write to when the message is sent; then
    // previous messages to the same client can be scanned and used as templates.

    return m;
}

nMessage& operator << ( nMessage& m, Message const & buffer )
{
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

nOPBDescriptorBase::~nOPBDescriptorBase()
{}

unsigned int nOPBDescriptorBase::GetObjectID ( Network::nNetObjectInit const & message )
{
    return message.object_id();
}

//! checks to run before creating a new object
//! @return true if all is OK
bool nOPBDescriptorBase::PreCheck( unsigned short id, nSenderInfo sender )
{
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
void nOPBDescriptorBase::PostCheck( nNetObject * object, nSenderInfo sender )
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
