
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

#define NO_MALLOC_REPLACEMENT

#include "tMemManager.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include "tArray.h"
#include <string.h>

void GrowingArrayBase::ComplainIfFull(){
    if (Len()>0)
        tERR_ERROR("Array should be empty.");
}

GrowingArrayBase::GrowingArrayBase(int firstsize,int size_of_T, bool useMalloc)
{
    // dump(low,dump,"con:size " << firstsize << ",element size " << size_of_T);
    len=firstsize;
    if (firstsize) size=firstsize;
    else             size=1;

    if ( useMalloc )
        base=malloc(size * size_of_T);
    else
        base=tNEW( char[size * size_of_T] );

    if(NULL==base) {
        tERR_ERROR("Error Allocating " << size_of_T*(size) << " bytes." );
    }
    for(int i=size*size_of_T-1;i>=0;i--)
        (reinterpret_cast<char *>(base))[i]=0;

}

GrowingArrayBase::~GrowingArrayBase(){
    tASSERT( base == NULL );
    // dump(very_low,flow,"des");
}

void GrowingArrayBase::Delete( bool useMalloc ){
    // dump(very_low,flow,"des");
    if ( useMalloc )
    {
        free(base);
    }
    else
    {
        delete[] (char*)base;
    }

    base = NULL;
    size = len = 0;
}

void GrowingArrayBase::ResizeBase(int i,int size_of_T, bool useMalloc){

    i++;

    // dump(very_low,flow,"Array-base resize");

    unsigned int oldsize=size;

    int size_a=i+(1<<12);
    int size_b=i+(i>>2);

    int new_size;

    if (size_a<size_b)
        new_size=size_a;
    else
        new_size=size_b;

    //  void *newbase=NULL;//realloc(base,size_of_T*(size));
    //  void *newbase=realloc(base,size_of_T*(size));
    //  if(NULL==newbase){

    void *newbase;
    if ( useMalloc )
        newbase = malloc(new_size * size_of_T);
    else
        newbase=tNEW( char[new_size * size_of_T] );


    if(NULL==newbase){
        tERR_ERROR("Error reallocating " << size_of_T*(new_size) << " bytes.");
    }
    else{
        memcpy(newbase,base,size_of_T*(oldsize));
        if ( useMalloc )
        {
            free(base);
        }
        else
        {
            delete[] (char*)base;
        }
    }
    //
    size = new_size;

    // clear the newly allocated memory
    char *start=(reinterpret_cast<char *>(newbase)) + oldsize*size_of_T;
    for(int j=(size-oldsize)*size_of_T-1;j>=0;j--)
        start[j]=0;

    base=newbase;
}











