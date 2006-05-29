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

#include "tMemStack.h"
#include "tArray.h"
#include <stdlib.h>

static int& ST_Size()

{
	#ifdef DEBUG
    static int st_size=10;
	#else
    static int st_size=1000;
	#endif
    return st_size;

}


class tMemStackItem
{
public:
    void* memory;
    int   size;

    tMemStackItem()
    {
        memory = NULL;
        size   = 0;
    }

    ~tMemStackItem()
    {
        if ( memory )
            free(memory);
    }

    void Alloc()
    {
        if ( ST_Size() > size )
        {
            size = ST_Size();
            if ( memory )
                free(memory);

            memory = malloc( size );


            for ( int i = size-1; i>=0; --i )

            {

                ((char*)(memory))[i] = 0;

            }

        }
    }
};

static tArray<tMemStackItem, true>& ST_Stack()

{

    static tArray<tMemStackItem, true> st_stack;

    return st_stack;

}



static int& ST_Index()

{

    static int st_index=0;

    return st_index;

}


static void st_Pop()
{
    --ST_Index();
}

tMemStackItem& tMemStack::Item() const
{
    return ST_Stack()[this->index];
}

tMemStack::tMemStack	(int minSize )
        : index( ST_Index()++ )
{
    if ( ST_Size() < minSize )
        ST_Size() = minSize;

    Item().Alloc();
}

tMemStack::~tMemStack	()
{
    st_Pop();

#ifdef DEBUG
    if( index != ST_Index() )
    {
        st_Breakpoint();
    }
#endif
}


// get the memory pointer
void* 	tMemStack::GetMem()			const
{
    return Item().memory;
}

// get the memory size
int	  	tMemStack::GetSize() 		const
{
    return Item().size;
}

// recreate the buffer a bit larger
void  	tMemStack::IncreaseMem()
{
    ST_Size() *= 2;

    Item().Alloc();
}

