/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2008  Manuel Moos (manuel@moosnet.de) and the AA Development Team

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

#include "rDisplayList.h"

#include "rScreen.h"

static rDisplayList * se_displayListAnchor = NULL;

rDisplayList::rDisplayList()
    : tListItem< rDisplayList >( se_displayListAnchor )
    , list_( 0 )
    , inhibit_( 0 )
    , filling_( false )
{
}

rDisplayList::~rDisplayList()
{
    Clear();

    tASSERT( !list_ );
}

// calls the display list, returns true if there was a list to call
bool rDisplayList::Call()
{
    tASSERT( !filling_ );

    if ( inhibit_ > 0 )
    {
        Clear();
        --inhibit_;
        return false;
    }

    if ( list_ )
    {
        glCallList( list_ );
        return true;
    }

    return false;
}

//! clears the display list and don't regenerate it for the next few calls
void rDisplayList::Clear( int inhibitGeneration )
{
    // clear the list
    if ( !filling_ && list_ )
    {
        glDeleteLists( list_, 1 );
        list_ = 0;
    }
    else
    {
        // clear it later
        if ( inhibitGeneration < 1 )
        {
            inhibitGeneration = 1;
        }
    }

    // memorize inhibit counter
    if ( inhibit_ < inhibitGeneration )
    {
        inhibit_ = inhibitGeneration;
    }
}

// clears all display lists
void rDisplayList::ClearAll()
{
#ifndef DEDICATED
    rDisplayList *run = se_displayListAnchor;
    while (run)
    {
        run->Clear();
        run = run->Next();
    }
#endif
}

//! constructor, automatically starting to fill teh list
rDisplayListFiller::rDisplayListFiller( rDisplayList & list )
    : list_( list )
{
    bool useList = sr_useDisplayLists && list_.inhibit_ == 0;
    if ( useList )
    {
        if ( !list_.list_ )
        {
            list_.list_=glGenLists(1);
        }
        glNewList(list_.list_,GL_COMPILE_AND_EXECUTE);
        list_.filling_ = true;
    }
}

rDisplayListFiller::~rDisplayListFiller()
{
    Stop();
}

//! stops filling the display list (done automatically on destruction)
void rDisplayListFiller::Stop()
{
    if ( list_.filling_ )
    {
        list_.filling_ = false;
        glEndList();
    }
}

static rCallbackBeforeScreenModeChange sr_unload( &rDisplayList::ClearAll );
