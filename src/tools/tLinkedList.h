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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_tLinkedList_H
#define ArmageTron_tLinkedList_H

#include "tError.h"

#include <stdlib.h>

class tListItemBase{
protected:
    tListItemBase  *next;
    tListItemBase **anchor;

public:
    typedef int (Comparator)( const tListItemBase* a, const tListItemBase* b );

    void Remove();
    void Insert(tListItemBase *&a);

    tListItemBase():next(NULL),anchor(NULL)                 {}
    tListItemBase(tListItemBase *&a):next(NULL),anchor(NULL){Insert(a);}
    virtual ~tListItemBase()                                {Remove();}

    //! returns the next list element
    tListItemBase *Next()                             {return next;}

    //! returns true if this object is in a list
    bool IsInList() const {return anchor;}

    int Len();
    void Sort( Comparator* comparator );
};

template < typename T, typename comparator >
int st_Compare( const tListItemBase* a, const tListItemBase* b )
{
    const T* A = static_cast<const T*>( a );
    const T* B = static_cast<const T*>( b );

    return comparator::Compare( A, B );
}


template <class T> class tListItem:public tListItemBase{
public:
    tListItem():tListItemBase()
    { 
        // this class only works under this condition:
        tASSERT( static_cast< tListItemBase * >( ( T * )(NULL)  ) == NULL );
    };
    tListItem(T *&a):tListItemBase(reinterpret_cast<tListItemBase*&>(a)){};
    T *Next(){return reinterpret_cast<T*>(next);}

    template< typename comparator >
    void Sort( )
    {
        tListItemBase::Sort( &st_Compare< T, comparator> );
    }

    void Insert(tListItem *&a)
    {
        tListItemBase::Insert( reinterpret_cast<tListItemBase*&>(a) );
    }

    void Insert(T *&a)
    {
        tListItemBase::Insert( reinterpret_cast<tListItemBase*&>(a) );
    }

    void Insert(tListItemBase *&a)
    {
        tListItemBase::Insert( a );
    }
};

#endif
