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

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

using namespace google::protobuf;

/*
Not defined here, but in nNetwork.cpp for access to the right static variables and macros.

nPBDescriptorBase::nPBDescriptorBase(unsigned short identification,
                                     const char * name, 
                                     bool acceptEvenIfNotLoggedIn )
*/

//! dumb streaming to message
void nPBDescriptorBase::DoStreamTo( Message const & in, nMessage & out ) const
{
    // get reflection interface
    const Reflection * reflection = in.GetReflection();
    const Descriptor * descriptor = in.GetDescriptor();

    // iterate over fields in ID order
    int count = descriptor->field_count();
    for( int i = 0; i < count; ++i )
    {
        FieldDescriptor const * field = descriptor->field( i );

        if ( ! field->is_required() )
        {
            // only required fields get streamed
            continue;
        }

        switch( field->cpp_type() )
        {
        case FieldDescriptor::CPPTYPE_INT32:
            out << reflection->GetInt32( in, field );
            break;
        case FieldDescriptor::CPPTYPE_UINT32:
            out.Write( reflection->GetUInt32( in, field ) );
            break;
        case FieldDescriptor::CPPTYPE_STRING:
            out << reflection->GetString( in, field );
            break;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            // todo: call function over corresponding descriptor
            nPBDescriptorBase::DoStreamTo( reflection->GetMessage( in, field ), out );
            break;

            // not yet implemented
        case FieldDescriptor::CPPTYPE_FLOAT:
        case FieldDescriptor::CPPTYPE_BOOL:
        case FieldDescriptor::CPPTYPE_ENUM:

            // explicitly unsupported:
        case FieldDescriptor::CPPTYPE_INT64:
        case FieldDescriptor::CPPTYPE_UINT64:
        case FieldDescriptor::CPPTYPE_DOUBLE:
        default:
            tERR_ERROR( "Unsupported field type." );
            break;
        }
    }
   
}

//! dumb streaming from message
void nPBDescriptorBase::DoStreamFrom( nMessage & in, Message & out ) const
{
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
