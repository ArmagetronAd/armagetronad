/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by Manuel Moos
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "tDecorator.h"

// *******************************************************************************
// *	
// *	Allocate
// *	
// *******************************************************************************
//!		
//!		@param	size	size of the memory block to allocate
//!		@param	classn	name of the class that is allocated
//!		@param	file	name of the file this call comes from
//!		@param	line	line in the file the call ocmes from
//!		@return		    the newly allocated block of memory, with space for decorators
//!		
// *******************************************************************************

void * tDecoratableManagerBase::Allocate( size_t size, const char * classn, const char * file, int line )
{
    // register object with manager
    if ( refCount_++ == 0 )
    {
        /// calculate offsets of decorators if required
        CalculateOffsets();
    }

    // allocate requested space plus overhead for decorators
    size_t overhead = GetSize();
#ifndef DONTUSEMEMMANAGER

    char * base = static_cast< char * > ( ::operator new( size + overhead, classn, file, line ) ) + overhead;
#else
    char * base = static_cast< char * > ( ::operator new( size + overhead ) ) + overhead;
#endif
    
    // call decoration constructors
    ConstructAll( base);

    return base;
}

// *******************************************************************************
// *	
// *	Free
// *	
// *******************************************************************************
//!		
//!		@param	ptr	    pointer to the memory area to be freed
//!		@param	classn	name of the class that is allocated
//!		@param	file	name of the file this call comes from
//!		@param	line	line in the file the call ocmes from
//!		
// *******************************************************************************

void tDecoratableManagerBase::Free( void * ptr, const char * classn, const char * file, int line )
{
    // call decoration destructors
    DestructAll( ptr );

    // deregister with manager
    --refCount_;

    // deallocate space and overhead
    size_t overhead = GetSize();
    char * base = ( ( char * ) ptr ) - overhead;

#ifndef DONTUSEMEMMANAGER
    ::operator delete( base, classn, file, line );
#else
    ::operator delete( base );
#endif
}

// *******************************************************************************
// *	
// *	Tests
// *	
// *******************************************************************************

class base
{
public:
    virtual ~base(){}

    DECORATABLE_BASE(base)
};

static tDecoratorPOD< base, int > decorator( 0 );

class derived: public base
{
public:
    ~derived(){}

    DECORATABLE(derived,base)
};

class dummy{};
void test_decorators()
    {
        tASSERT( tIsDecoratable< base >::DECORATABLE );
        tASSERT( !tIsDecoratable< dummy >::DECORATABLE );

        base * p = new base;
        int * decoration = &decorator.Get( *p );
        delete p;
        p = new derived;
        decoration = &decorator.Get( *p );
        delete p;

        // shouldn't compile
        // tDecoratable decoratable;

        // tDecoratable * d2 = &decoratable;
//        d2 = tNEW(tDecoratable);
//        delete d2;
//        d2 = new tDerivedDecoratable;
//        delete d2;
    }
