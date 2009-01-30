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

#include "nBinary.h"

//! writes a variable width unsigned integer, using 1 to 5 bytes.
void nBinaryWriter::WriteVarUInt( unsigned int value )
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

//! writes a variable width signed integer
void nBinaryWriter::WriteVarSInt( int value )
{
    if ( value > 0 )
    {
        WriteVarUInt( value << 1 );
    }
    else
    {
        WriteVarUInt( ( (-1-value) << 1 ) | 1 );
    }
}

//! reads a variable width int, using 1 to 5 bytes.
unsigned int nBinaryReader::ReadVarUInt()
{
    unsigned int ret = 0;
    unsigned char c = ReadByte();
        
    // read as long as the MSB is set
    unsigned int shift = 0;
    while( c & 0x80 )
    {
        ret |= (c & 0x7f) << shift;
        shift += 7;
        c = ReadByte();
    }

    // read last value with msb cleared.
    ret |= c << shift;

    return ret;
}

//! reads a variable width signed integer
int nBinaryReader::ReadVarSInt()
{
    unsigned int value = ReadVarUInt();
    if ( value & 1 )
    {
        // negative number
        return -1 - ( (value & ~1) >> 1 );
    }
    else
    {
        // positive number
        return value >> 1;
    }
}
