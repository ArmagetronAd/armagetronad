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

// read/write operators for protocol buffers
nMessage& operator >> ( nMessage& m, google::protobuf::Message & buffer )
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

nMessage& operator << ( nMessage& m, google::protobuf::Message const & buffer )
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
