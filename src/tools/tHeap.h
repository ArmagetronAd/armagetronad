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

#ifndef ArmageTron_HEAP_H
#define ArmageTron_HEAP_H

#include "tList.h"
#include "defs.h"

/*
  Heap data structure
*/

#ifdef DEBUG_EXPENSIVE
#define EVENT_DEB
#endif

// #define HEAP_DEB

class tHeapElement;

class tHeapBase:private tList<tHeapElement>{
    friend class tHeapElement;
protected:
    int  Lower(int i){ // the element below i in the heap
        if(i%2==0)  // just to make sure what happens; you never know what 1/2 is..
            return i/2-1;
        else
            return (i-1)/2;
    }

int Len()const { return tList<tHeapElement>::Len(); }
    tHeapElement* operator()( int i ) { return const_cast< tHeapElement* >( tList<tHeapElement>::operator()(i) ); }
    const tHeapElement* operator()( int i )const { return tList<tHeapElement>::operator()(i); }

    bool SwapIf(int i,int j); // swaps heap elements i and j if they are
    // in wrong order; only then, TRUE is returned.
    // i is assumed to lie lower in the heap than j.

    void SwapDown(int j); // swap element j as far down as it may go.
    void SwapUp(int i);   // swap element j as far up as it must.

    //#ifdef EVENT_DEB
    void CheckHeap(); // checks the heap structure
    //#endif

    void Insert(tHeapElement *e);  // starts to manage object e
    void Remove(tHeapElement *e);  // stops (does not delete e)
    tHeapElement * Remove(int i);     // stops to manage object i, wich is returned.
    void Replace(int i);  // puts element i to the right place (if the
    // rest of the heap has correct order)
    void Replace(tHeapElement *e);
public:
    static int  UpperL(int i){return 2*i+1;} // the elements left and
    static int  UpperR(int i){return 2*i+2;} // right above i

    tHeapBase(){}
    ~tHeapBase();
};


// the heap elements.

class tHeapElement{
    friend class tHeapBase;

public:
    tHeapElement():hID(-1),value_(0){}
    virtual ~tHeapElement();

    REAL Val() const {return value_;}
    void SetVal( REAL value, tHeapBase& heap );
    void RemoveFromHeap();

protected:
    virtual tHeapBase *Heap() const = 0; //{return NULL;} // in wich heap are we?

protected:
private:
    int  hID;         // the id in the heap
    REAL value_;      // our value. lower values are lower in the heap.
};


template<class T> class tHeap: public tHeapBase{
public:
    tHeap(){};
    ~tHeap(){};

    void Insert(T *e){tHeapBase::Insert(e);}  // starts to manage object e
    void Remove(T *e){tHeapBase::Remove(e);}  // stops (does not delete e)
    void Replace(T *e){tHeapBase::Replace(e);}
    T * Remove(int i){return static_cast<T*> (tHeapBase::Remove(i));}

    T * operator()(int i){return static_cast< T *>(tHeapBase::operator()(i));}
    const T * operator()(int i) const {return static_cast<const T *>(tHeapBase::operator()(i));}
    T * Events(int i){return static_cast<T *>(tHeapBase::operator()(i));}

    int Len() const {return tHeapBase::Len();}

    tHeapBase * operator &(){return this;}
};

#endif

