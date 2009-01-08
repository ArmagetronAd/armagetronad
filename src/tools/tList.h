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

#ifndef ArmageTron_LIST_H
#define ArmageTron_LIST_H

#include "tArray.h"
#include "tSafePTR.h"
#include <new>
#include <stdlib.h>

namespace referencing
{
inline void AddReference( void* )
{
}

inline void ReleaseReference( void* )
{
}
}

#define tDECLARE_REFOBJ(T) \
class T;\
namespace referencing	\
{\
  void AddReference( T* );\
  void ReleaseReference( T* );\
}

#define tDEFINE_REFOBJ(T) \
namespace referencing \
{\
  void AddReference( T* t)\
  {\
    tASSERT(t);\
	t->AddRef();\
  }\
  void ReleaseReference( T* t)\
   {\
	 tASSERT(t);\
	 t->Release();\
   }\
}

tDECLARE_REFOBJ( nNetObject )
tDECLARE_REFOBJ( ePoint )
tDECLARE_REFOBJ( eHalfEdge )
tDECLARE_REFOBJ( eFace )

// a usefull class of lists

template < class T >
class tReferencer
{
public:
    static void AddReference( T* t )
    {
        referencing::AddReference( t );
    }

    static void ReleaseReference( T* t )
    {
        referencing::ReleaseReference( t );
    }
};

template <class T, bool MALLOC=false, bool REFERENCE=false> class tList:public tArray<T *, MALLOC> {
    //  friend T;
    int offset;

    // Array<T *> list;
public:

    ~tList(){
        for(int i=this->Len()-1;i>=0;i--)
            (reinterpret_cast<int *>((*this)(i)))[offset]=-1;
    }

    tList(int size=0):tArray<T*, MALLOC>(size){}

    void Add(T *t,int &idnum){
        offset=&idnum-(reinterpret_cast<int *>(t));

        if (idnum<0){    // tEventQueue relies on the fact that we put t in
            idnum=this->Len();   // the last place.
            (*this)[idnum]=t;
            if ( REFERENCE )
            {
                tReferencer< T >::AddReference( t );
            }
        }
    }

    void Add( T* t )
    {
        Add( t, t->ListIDRef() );
    }

    void Remove(T *t,int &idref)
    {
        int idnum = idref;
        idref = -1;

        // con << "offset=" << offset << '\n';
        if ( idnum>=0 ){
#ifdef DEBUG
            if (idnum>=this->Len())
                tERR_ERROR_INT("Corrupted list structure!");

            T *test=(*this)(idnum);
            if (test!=t)
                tERR_ERROR_INT("Corrupted list structure!");
#endif
            // the check for Len() is done, since this may be
            // called on an allready descructed list.
            if ( this->Len() > idnum+1 )
            {
                T *other=(*this)(this->Len()-1);
                tASSERT( other );
                (*this)(idnum)=other;
                int &other_id=(reinterpret_cast<int *>(other))[offset];
                tASSERT( other_id == this->Len()-1 );
                other_id=idnum;
            }
            (*this)[this->Len()-1] = NULL;

            SetLen(this->Len()-1);

            if ( REFERENCE )
            {
                tReferencer< T >::ReleaseReference( t );
            }
        }

        //		tASSERT( idref == -1 );
    }

    void Remove( T* t )
    {
        Remove( t, t->ListIDRef() );
    }

    //! fast data swap with other list
    void Swap( tList & other )
    {
        tArray<T *, MALLOC>::Swap( other );

        ::Swap( offset, other.offset );
    }
private:
    // forbid copying
    tList( tList const & );
    tList & operator = ( tList const & );
};

class tListMember
{
public:
    int ListID(){return listID_;}
    int& ListIDRef(){return listID_;}
    tListMember():listID_(-1){};
private:
    int listID_;
};

#endif
