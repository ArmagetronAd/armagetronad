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

// #include "defs.h"
#include <vector>
#include "tError.h"
// #include "tSafePTR.h"

template<class T> class tArray: public std::vector<T> {
protected:
    void Swap( tArray & other )
    {
        std::swap(*this, other);
    }

    void Clear(){
        this->clear();
    }

    void CopyFrom(const tArray &A){
        operator=(A);
    }

public:
    typedef std::vector<T> BASE;

    int Len() const
    {
        return this->size();
    }

    void SetLen(int i){
        while(i > this->size())
            this->push_back(T());
        while(i < this->size())
            this->pop_back();
    }

    ~tArray(){
        tERR_FLOW_LOW();
        this->clear();
    }

    tArray(int firstsize=0)
    :std::vector<T>(firstsize) {
    }

    tArray(const tArray &A)
    :std::vector<T>(A){
    }

    typename BASE::reference operator[](int i) {
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

        return BASE::operator[](i);
    }

    // Allow to READ from a const object. 
    T const& operator[](int i) const {
        tASSERT( i >= 0 && i < Len() );

        return BASE::operator[](i);
    };

    T const& operator()(int i) const{
        tASSERT( i >= 0 && i < Len() );

        return BASE::operator[](i);
    }
 
    typename BASE::reference operator()(int i){
        tASSERT( i >= 0 && i < Len() );

        return BASE::operator[](i);
    }

    T const* operator+(int i) const{
    #ifdef DEBUG
        if (i<0) {tERR_ERROR_INT("Range error;accesed negative element " << i )}
        if (i>=Len())
        {tERR_ERROR_INT("Range error;accesed element "
                            << i <<" of maximal " <<Len())}
    #endif

        return &BASE::operator[](i);
    }

    T * operator+(int i) {
    #ifdef DEBUG
        if (i<0) {tERR_ERROR_INT("Range error;accesed negative element " << i )}
        if (i>=Len())
        {tERR_ERROR_INT("Range error;accesed element "
                            << i <<" of maximal " <<Len())}
    #endif

        return &BASE::operator[](i);
    }

    const tArray<T> &operator=(const tArray<T> &A){
        BASE::operator=(A);

        return *this;
    }

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




