/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005 by the AA DevTeam (see the file AUTHORS(.txt)
in the main source directory)

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

#include "tCrypto.h"
#include "tString.h"

bool operator==( const tChecksum & a, const tChecksum & b )
{
    for ( int i = 15; i >= 0; i-- )
        if ( a[i] != b[i] )
            return false;
    return true;
}

bool operator!=( const tChecksum & a, const tChecksum & b )
{
    return !(a == b);
}

std::ostream & operator <<( std::ostream &s, const tChecksum & checksum )
{
    for( int i = 0; i < 16; ++i )
    {
        s << static_cast< unsigned int >( checksum[i] );
        if ( i != 15 )
            s << ' ';
    }
    return s;
}

std::istream & operator >>( std::istream & s, tChecksum & checksum )
{
#define CHECK() if ( s.eof() || !s.good() ) { checksum.Clear(); return s; }
    checksum.Clear();
    for ( int i = 0; i < 16; ++i )
    {
        CHECK();
        unsigned int x;
        s >> x;
        checksum[ i ] = x;

        // Read space
        if ( x != 15 )
        {
            CHECK();
            s.get();
        }
    }
    return s;
#undef CHECK
}
