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

#ifndef ArmageTron_nBINARY_H
#define ArmageTron_nBINARY_H

#include "nNetwork.h"

// Untility classes for compressed binary writing and reading

//! binary writer class, writes into a send buffer
class nBinaryWriter
{
public:
    typedef nSendBuffer::Buffer Buffer;
    explicit nBinaryWriter( Buffer & target )
    : target_( target ), writeIndex_( target.Len() )
    {
    }

    //! writes a single byte
    void WriteByte( unsigned char s )
    {
        target_[ writeIndex_++ ] = s;
    }

    //! writes a fixed width short
    void WriteShort( unsigned short s )
    {
        unsigned char hi = s >> 8;
        unsigned char lo = s;

        WriteByte( hi );
        WriteByte( lo );
    }

    //! writes a variable width int, using 1 to 5 bytes.
    void WriteVarInt( unsigned int value )
    {
        // write lsbs first with msb set to indicate
        // stuff comes after it
        while( value >= 0x80 )
        {
            WriteByte( ( value & 0x7f ) | 0x80 );
            value >>= 7;
        }

        // write the remainder with msb clear
        WriteByte( value );
    }
private:
    Buffer & target_;
    size_t   writeIndex_;
};

//! binary reader class, reads from memory
class nBinaryReader
{
public:
    explicit nBinaryReader( unsigned char const * & read, unsigned char const * end )
    : read_( read ), end_( end )
    {
    }

    //! writes a single byte
    unsigned char ReadByte()
    {
        if( read_ >= end_ )
        {
            throw nKillHim();
        }

        return *(read_++);
    }

    //! reads a fixed width short
    unsigned short ReadShort()
    {
        unsigned short hi = ReadByte();
        unsigned short lo = ReadByte();
        
        return lo + ( hi << 8 );
    }

    //! reads a variable width int, using 1 to 5 bytes.
    unsigned int ReadVarInt()
    {
        unsigned int ret = 0;
        unsigned char c = ReadByte();
        
        // read as long as the MSB is set
        while( c & 0x80 )
        {
            ret |= c & 0x7f;
            ret <<= 7;
            c = ReadByte();
        }

        // read last value with msb cleared.
        ret |= c;

        return ret;
    }
private:
    //! current read position
    unsigned char const * & read_;

    //! byte just after the message end
    unsigned char const * end_;
};

#endif
