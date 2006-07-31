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

#include "tLinkedList.h"

void tListItemBase::Remove(){
    if (anchor){
        *anchor      = next;
        if (next)
            next->anchor = anchor;
        anchor       = NULL;
        next         = NULL;
    }
}

void  tListItemBase::Insert(tListItemBase *&a){
    if (anchor)
        Remove();
    anchor = &a;
    next   =  a;
    a      =  this;
    if (next)
        next->anchor = &next;
}

int  tListItemBase::Len(){
    int ret=0;
    tListItemBase* x=this;
    while (x){
        ret++;
        x = x->next;
    }
    return ret;
}

void tListItemBase::Sort( Comparator* compare )
{
    // early return statements: empty list or single element in list
    if ( !this || !next )
    {
        return;
    }

    tListItemBase* middle = this;
    {
        // locate the middle of the list
        tListItemBase* run = *anchor;
        while ( run )
        {
            middle = middle->next;
            run 	 = run->next;
            if ( run )
            {
                run = run->next;
            }
        }
    }

    // split the list in the middle
    *middle->anchor = NULL;
    middle->anchor = &middle;

    // retrieve the anchor of the first half list
    tListItemBase*& first = *anchor;

    // sort the two half lists
    first->Sort( compare );
    middle->Sort( compare );

    // merge the lists
    {
        tListItemBase** run = &first;
        while ( middle )
        {
            // find the correct place for middle
            while ( *run && compare( *run, middle ) > 0 )
                run = &(*run)->next;

            // remove middle from the second list; care needs to be taken because middle->remove() would modify middle.
            tListItemBase* insert = middle;
            insert->Remove();

            // insert it into the first list
            insert->Insert( *run );
            run = &insert->next;
        }
    }

    // done!
}

