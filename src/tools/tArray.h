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

#ifndef ArmageTron_ARRAY_H
#define ArmageTron_ARRAY_H

#include "defs.h"
#include <new>
#include "tError.h"
#include "tSafePTR.h"


class GrowingArrayBase {
    int len;    // current logical size
    int size;   // current size in memory
    void *base; // start of memory block

    const GrowingArrayBase & operator=(const GrowingArrayBase &);
    GrowingArrayBase(GrowingArrayBase &);

protected:
    void ResizeBase(int i,int size_of_T, bool useMalloc);
    void *Base() const {return base;}

    //! fast data swap with other array
    void Swap( GrowingArrayBase & other );

    GrowingArrayBase(int firstsize,int size_of_T, bool useMalloc);
    void Delete( bool useMalloc );
    ~GrowingArrayBase();

    /*
    #ifdef tSAFEPTR
    void SetSize(int s){size=s;}
    void SetBase(void *b){base=b;}
    #endif
    */

public:
    void ComplainIfFull();

    void SetLen(int i){len=i;}
    int Len()const {return len;}
    int  Size() const {return size;}
};


template<class T, bool MALLOC=false> class tArray: public GrowingArrayBase {
protected:
    void Init(){
        int i;
        for(i=Size()-1;i>=0;i--)
            new(reinterpret_cast<T *>(Base())+i) T();
    }

    void resize(int i){
        int oldsize=Size();
        ResizeBase(i,sizeof(T),MALLOC);
        // dump(low,flow,"Array resize from " << oldsize << " to " << Size());
        for(i=Size()-1;i>=oldsize;i--)
            new(reinterpret_cast<T *>(Base())+i) T();
    }

    //! fast data swap with other array
    void Swap( tArray & other )
    {
        GrowingArrayBase::Swap( other );
    }

    void Clear(){
        int i;
        for(i=Size()-1;i>=0;i--){
            // dump(low,flow,"i=" << i);
            ((reinterpret_cast<T *>(Base()))+i)->~T();
        }
        Delete(MALLOC);
    }


    void CopyFrom(const tArray &A){
        int i;
        for(i=Len()-1;i>=0;i--)
            new(reinterpret_cast<T *>(Base())+i) T(A(i));
        for(i=Len();i<Size();i++)
            new(reinterpret_cast<T *>(Base())+i) T();
    }

public:
    void SetLen(int i){
        GrowingArrayBase::SetLen(i);
        if (i>Size()) resize(i);
    }

    ~tArray(){
        tERR_FLOW_LOW();
        Clear();
        Delete(MALLOC);
    }


    tArray(int firstsize=0)
            :GrowingArrayBase(firstsize,sizeof(T),MALLOC) {
        // dump(low,flow,"con:size " << firstsize);
        Init();
    };


    tArray(const tArray &A)
            :GrowingArrayBase(A.Len(),sizeof(T),MALLOC){
        CopyFrom(A);
    };


    T& operator[](int i) {
#ifdef DEBUG
        if (i<0) {
            tERR_ERROR_INT("Range error;accesed negative element " << i );
        }
#endif
        if (i>=Len())
        {
            SetLen(i+1);
        }

        //    dump(low,flow,"[" << i << "]" << "=" << ((T *)Base())[i] << '\n');

        return((reinterpret_cast<T *>(Base()))[i]);
    };

    // Allow to READ from a const object. 
    T const& operator[](int i) const {
        tASSERT( i >= 0 && i < Len() );

        return((reinterpret_cast<T *>(Base()))[i]);
    };

    T& operator()(int i) const{
        tASSERT( i >= 0 && i < Len() );

        return((reinterpret_cast<T *>(Base()))[i]);
    };

    T* operator+(int i) const{
    #ifdef DEBUG
        if (i<0) {tERR_ERROR_INT("Range error;accesed negative element " << i )}
        if (i>=Len())
        {tERR_ERROR_INT("Range error;accesed element "
                            << i <<" of maximal " <<Len())}
    #endif

        return(reinterpret_cast<T *>(Base())+i);
    };

    const tArray<T> &operator=(const tArray<T> &A){

        Clear();
        SetLen(A.Len());
        CopyFrom(A);

        return *this;
    };

    void RemoveAt( int index )
    {
        int newLen = this->Len()-1;
        T keep = (*this)[ index ];
        if ( index < newLen )
            (*this)[ index ] = (*this)[ newLen ];
        this->SetLen( newLen );
    }

    bool Remove( const T& t )
    {
        for ( int i = this->Len()-1; i >= 0; --i )
        {
            if ( (*this)[i] == t )
            {
                this->RemoveAt( i );
                return true;
            }
        }

        return false;
    }

    void Insert( const T& t )
    {
        SetLen( this->Len()+1 );
        (*this)[ this->Len() -1 ] = t;
    }
};


#endif // _ARRAY_H_




