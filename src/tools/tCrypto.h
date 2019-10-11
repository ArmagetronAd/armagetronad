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

#ifndef TCRYPTO_H_5770T48R
#define TCRYPTO_H_5770T48R

#include "md5.h"
#include "tError.h"
#include <iostream>

class tChecksum
{
public:
    tChecksum()
    {
        Clear();
    }

    md5_byte_t operator[]( int i ) const
    {
        tASSERT( i >= 0 && i < 16 );
        return content[i];
    }

    md5_byte_t & operator[]( int i )
    {
        tASSERT( i >= 0 && i < 16 );
        return content[i];
    }

    void Clear()
    {
        memset( &content, 0, sizeof(content));
    }

    md5_byte_t content[16];
};

bool operator==( const tChecksum & a, const tChecksum & b );
bool operator!=( const tChecksum & a, const tChecksum & b );

std::ostream & operator <<( std::ostream & s, const tChecksum & checksum );
std::istream & operator >>( std::istream & s, tChecksum & checksum );

#endif /* end of include guard: TCRYPTO_H_5770T48R */
