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

#include "tHeap.h"
#include "tConsole.h"
#include <iostream>

/* ************************
   Heap data structure
   ************************ */


bool tHeapBase::SwapIf(int i,int j)
{
    if (i==j || i<0) return false; // safety

    tHeapElement *e1=operator()(i),*e2=operator()(j);

    tASSERT( e1->hID == i );
    tASSERT( e2->hID == j );

    if (e1->Val() > e2->Val()){
        ::Swap(tList< tHeapElement >::operator()(i),tList< tHeapElement >::operator()(j));
        e1->hID=j;
        e2->hID=i;
        return true;
    }
    else
        return false;
}

tHeapBase::~tHeapBase()
{
    while(Len() > 0)
        Remove(0);
}

//#ifdef EVENT_DEB
void tHeapBase::CheckHeap(){
    for(int i=Len()-1;i>0;i--){
        tHeapElement *current=operator()(i);
        tHeapElement *low=operator()(Lower(i));
        if (Lower(UpperL(i))!=i || Lower(UpperR(i))!=i)
            tERR_ERROR_INT("Error in lower/upper " << i << "!");

        if (low->Val() > current->Val() )
            tERR_ERROR_INT("Heap structure corrupt!");

        if ( current->hID != i )
            tERR_ERROR_INT("Heap list structure corrupt!");
    }
}
//#endif

void tHeapBase::SwapDown(int j){
    int i=j;
    // e is now at position i. swap it down
    // as far as it goes:
    do{
        j=i;
        i=Lower(j);
    }
    while(SwapIf(i,j)); // mean: relies on the fact that SwapIf returns -1
    // if i<0.

#ifdef EVENT_DEB
    CheckHeap();
#endif
}

void tHeapBase::SwapUp(int i){
#ifdef EVENT_DEB
    //  static int su=0;
    //  if (su%100 ==0 )
    //    con << "su=" << su << '\n';
    //  if (su > 11594 )
    // con << "su=" << su << '\n';
    //  su ++;
#endif

    int ul,ur;
    bool goon=1;
    while(goon && UpperL(i)<Len()){
        ul=UpperL(i);
        ur=UpperR(i);
        if(ur>=Len() ||
                operator()(ul)->Val() < operator()(ur)->Val() ){
            goon=SwapIf(i,ul);
            i=ul;
        }
        else{
            goon=SwapIf(i,ur);
            i=ur;
        }
    }

#ifdef EVENT_DEB
    CheckHeap();
#endif

}

void tHeapBase::Insert(tHeapElement *e){
#ifdef EVENT_DEB
    CheckHeap();
#endif

    Add(e,e->hID); // relies on the implementation of List: e is
    // put to the back of the heap.
    tASSERT(e->hID == Len()-1);
    SwapDown(Len()-1);  // bring it to the right place

#ifdef EVENT_DEB
    CheckHeap();
#endif
}

void tHeapBase::Remove(tHeapElement *e){
    int i=e->hID;

#ifdef EVENT_DEB
    CheckHeap();
#endif

    // element
    if(i<0 )
    {
#ifdef DEBUG
        static bool warn = true;
        if ( warn )
        {
            tERR_WARN("Element to be removed from heap was already removed. Unless there is a fatal exit in process, this is not right.");
            warn=false;
        }
#endif
        return;
    }

    if( this != e->Heap())
        tERR_ERROR_INT("Element is not in this heap! (Note: this usually is a followup error when the system fails to recover from another error. When reporting, please also include whatever happened before this.)");

    Remove(i);

#ifdef EVENT_DEB
    CheckHeap();
#endif
}

void tHeapBase::Replace(int i){
    if (i>=0 && i < Len()){
        if (i==0 || operator()(i)->Val() > operator()(Lower(i))->Val() )
            SwapUp(i);          // put it up where it belongs
        else
            SwapDown(i);

#ifdef EVENT_DEB
        CheckHeap();
#endif
    }
}

void tHeapBase::Replace(tHeapElement *e){
    int i=e->hID;

    if(i<0 || this != e->Heap())
        tERR_ERROR_INT("Element is not in this heap! (Note: this usually is a followup error when the system fails to recover from another error. When reporting, please also include whatever happened before this.)");

    Replace(i);

#ifdef EVENT_DEB
    CheckHeap();
#endif
}

tHeapElement * tHeapBase::Remove(int i){
#ifdef EVENT_DEB
    CheckHeap();
#endif

    if (i>=0 && i<Len())
    {
        tHeapElement *ret=operator()(i);

        tASSERT( ret->hID == i );

        tList<tHeapElement>::Remove(ret,ret->hID);

        // now we have an misplaced element at pos i. (if i was not at the end..)
        if (i<Len())
            Replace(i);

#ifdef EVENT_DEB
        CheckHeap();
#endif

        return ret;
    }
    return NULL;
}



/* ************************
   Events:
   ************************ */

tHeapElement::~tHeapElement()
{
    tASSERT( hID < 0 );
}

void tHeapElement::RemoveFromHeap(){
    tASSERT_THIS();

    if (hID>=0)
    {
        tASSERT( Heap() );
        Heap()->Remove(this);
    }
}

void tHeapElement::SetVal( REAL value, tHeapBase& heap )
{
    tASSERT( !Heap() || Heap() == &heap );

    this->value_ = value;

    if ( hID>=0 )
    {
        tASSERT( heap( hID ) == this );

        heap.Replace( this );
    }
    else
    {
        heap.Insert( this );
    }
}

// in wich heap are we?
tHeapBase *tHeapElement::Heap() const
{
    tASSERT( 0 );
    return NULL;
}


