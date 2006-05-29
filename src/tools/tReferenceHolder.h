/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_REFERENCEHOLDER_H
#define ArmageTron_REFERENCEHOLDER_H

#include "tArray.h"
#include "tSafePTR.h"

template< class T > class tReferenceHolder
{
    tArray< tControlledPTR< T > > references_;

public:
    void ReleaseAll()
    {
        for ( int i = references_.Len()-1; i >= 0; --i )
        {
            references_[i] = NULL;
        }

        references_.SetLen( 0 );
    }

    void Add( T* obj )
    {
        for ( int i = references_.Len()-1; i >= 0; --i )
        {
            if ( references_[i] == obj )
            {
                return;
            }
        }

        references_[ references_.Len() ] = obj;
    }

    void Remove( T* obj )
    {
        int iLast = references_.Len()-1;
        for ( int i = iLast; i >= 0; --i )
        {
            if ( references_[ i ] == obj )
            {
                references_[ i ] = references_[ iLast ];
                references_[ iLast ] = NULL;
                references_.SetLen( iLast );
            }
        }
    }
};

#endif
