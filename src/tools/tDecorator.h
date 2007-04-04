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

#ifndef ArmageTron_DECORATOR_H
#define ArmageTron_DECORATOR_H

#include "tMemManager.h"
#include "tLinkedList.h"
#include "tError.h"

//! dummy class that decoratable objects have a dummy conversion operator to, so template magic can detect whether a class is decoratable or not
class tDecoratableIndicator{};

//! compile time test class that determines whether a class is decoratable
template <class Decorated> class tIsDecoratable
{
private:
    struct Ret{ char a; char b; };
    static char TestDecoratable(tDecoratableIndicator const &);
    static Decorated & decorated_;

public:
    // only public to shut up compiler warnings
    static Ret TestDecoratable(...);

    //! this enum is the flag. Use tIsDecoratable<CLASS>::DECORATABLE to find out
    //! whether CLASS is decoratable or not.
    enum { DECORATABLE=(sizeof(TestDecoratable( decorated_ ) ) <= sizeof( char) ) };
};

// *************************************************************************************

class tDecoratableManagerBase;

//! base class for decorators
class tDecoratorBase: public tListItem< tDecoratorBase >
{
    friend class tDecoratableManagerBase;
protected:
    size_t offset_;
    tDecoratorBase():offset_(0xbaad){};
private:
    virtual size_t GetSize() const = 0;
    virtual void   Construct( void * decoration ) const = 0;
    virtual void   Destruct( void * decoration ) const = 0;

    tDecoratorBase( tDecoratorBase const & );
    tDecoratorBase & operator =( tDecoratorBase const & );
};


// *************************************************************************************

//! base class for managers of decorated classes
class tDecoratableManagerBase
{
public:
    tDecoratableManagerBase()
            : base_( 0 ), refCount_( 0 ), decorators_( 0 )
            , updateRequired_( true ), offset_( 0 )
    {
    }

    //! returns the total allocated size of decorations
    size_t GetSize() const
    {
        tASSERT( !updateRequired_ );

        return offset_;
    }

    //! allocates some byes of decorator space, returns the offset to it
    size_t Reserve( size_t allocate, size_t alignment = 4 )
    {
        offset_ += allocate;
        offset_ = (( offset_ + alignment - 1 ) / alignment ) * alignment;
        return offset_;
    }

    void AddDecorator( tDecoratorBase & decorator )
    {
        tASSERT( refCount_ == 0 );

        // add decorator to list and request
        decorator.Insert( decorators_ );
        updateRequired_ = true;
    }

    void CalculateOffsets()
    {
        // early exit if there is nothing to od
        if ( !updateRequired_ )
            return;

        // get start offset from base class
        offset_ = 0;
        if ( base_ )
        {
            base_->CalculateOffsets();
            offset_ = base_->offset_;
        }

        // iterate over listed decorators
        tDecoratorBase * run = decorators_;
        while ( run )
        {
            run->offset_ = Reserve( run->GetSize() );
            run = run->Next();
        }

        updateRequired_ = false;
    }

    void ConstructAll( void * decorated )
    {
        if ( base_ )
        {
            base_->ConstructAll( decorated );
        }

        // iterate over listed decorators
        tDecoratorBase * run = decorators_;
        while ( run )
        {
            void * decoration = static_cast< char * >( decorated ) - run->offset_;
            run->Construct( decoration );
            run = run->Next();
        }
    }

    void DestructAll( void * decorated )
    {
        if ( base_ )
        {
            base_->DestructAll( decorated );
        }

        // iterate over listed decorators
        tDecoratorBase * run = decorators_;
        while ( run )
        {
            void * decoration = static_cast< char * >( decorated ) - run->offset_;
            run->Destruct( decoration );
            run = run->Next();
        }
    }

    //! reserve space for an object of class Decorated, prepended with space for its decorators
    void * Allocate( size_t size, const char * classn, const char * file, int line );

    //! frees space reserved by Reseve() again
    void Free( void * ptr, const char * classn, const char * file, int line );
protected:
    tDecoratableManagerBase * base_; //! manager of the base class
    size_t refCount_;                //! number of objects of this
private:
    tDecoratableManagerBase( tDecoratableManagerBase const & );
    tDecoratableManagerBase & operator =( tDecoratableManagerBase const & );

    tDecoratorBase * decorators_;    //! anchor of the decorators managed by this managers


    bool   updateRequired_;          //! flag indicating whether the following information needs an update
    size_t offset_;                  //! offset of the managed decorator into the decorator space
};

//! manager class for decoratable classes
template<class Decorated> class tDecoratableManager: public tDecoratableManagerBase
{
public:
    //! reserve space for an object of class Decorated, prepended with space for its decorators
    static void * Allocate( size_t size, const char * classn, const char * file, int line );
    static void * Allocate( size_t size );

    //! frees space reserved by Reseve() again
    static void Free( void * ptr, const char * classn, const char * file, int line );
    static void Free( void * ptr );

    //! returns the static instance of this class
    static tDecoratableManager & GetManager()
    {
        static tDecoratableManager manager;
        return manager;
    };

private:
    //! typedef for the class that is registered with the decorator system
    typedef typename Decorated::DecoratableBase_ DecoratedBase;

    template< bool > struct tBoolToType{};

    tDecoratableManagerBase * GetBase( tBoolToType< true > )
    {
#ifdef DEBUG
        // check whether the beginnings of this and the base class are the same; the
        // offset calculation relies on that.
        Decorated * d = NULL;
        DecoratedBase * b = d;
        tASSERT( b == NULL );
#endif

        return &tDecoratableManager< DecoratedBase >::GetManager();
    }

    tDecoratableManagerBase * GetBase( tBoolToType< false > )
    {
        return NULL;
    }

    tDecoratableManager()
    {
        base_ = GetBase( tBoolToType< tIsDecoratable< DecoratedBase >::DECORATABLE >() );
    }
};

// *************************************************************************************

//! decorates a decoratable object with a decoration
template< class DecoratedDerived, class Decoration > class tDecorator: public tDecoratorBase
{
public:
    //! typedef for the class that is registered with the decorator system
    typedef typename DecoratedDerived::Decoratable_ Decorated;

    //! constructor, allocating the space
    tDecorator()
    {
        tDecoratableManager< Decorated > & manager = tDecoratableManager< Decorated >::GetManager();

        // insert into management list
        manager.AddDecorator( *this );
    }

    //! gets the decorator
    Decoration & Get( Decorated & object )
    {
        // pointer magic to get to the place of the decoration in decorator spacew
        char * endOfPointerSpace = static_cast< char * >( static_cast< void * >( &object ) );
        char * allocatedSlot     = endOfPointerSpace - offset_;
        Decoration * decoration = static_cast< Decoration * >( static_cast< void * >( allocatedSlot ) );

        return * decoration;
    }
private:
    virtual size_t GetSize() const { return sizeof( Decoration ); }

    virtual void Construct( void * decoration ) const
    {
        new(decoration) Decoration;
    }

    virtual void Destruct( void * decoration ) const
    {
        static_cast< Decoration * >( decoration )->~Decoration();
    }
};

template< class DecoratedDerived, class Decoration > class tDecoratorPOD: public tDecorator< DecoratedDerived, Decoration >
{
public:
    //! constructor, allocating the space
    tDecoratorPOD( Decoration const & value )
            : value_ ( value )
    {
    }

private:
    tDecoratorPOD();

    Decoration value_;

    virtual void Construct( void * decoration ) const
    {
        new(decoration) Decoration;
        *static_cast< Decoration * >( decoration ) = value_;
    }
};

//! macro making a class a decoratable class. If you end up on one of the macro usages in
//! a debugger, just "step into" the function to get to the true implementation, the macro
//! only is a dumb wrapper.
#define DECORATABLE(CLASS,BASE)                                \
public:                                                        \
operator tDecoratableIndicator & () const;                     \
typedef BASE DecoratableBase_;                                 \
typedef CLASS Decoratable_;                                    \
void * operator new    ( size_t size ) THROW_BADALLOC          \
{                                                              \
    return tDecoratableManager< CLASS >::Allocate( size );     \
}                                                              \
void   operator delete ( void * ptr ) THROW_NOTHING            \
{                                                              \
    tDecoratableManager< CLASS >::Free( ptr);                  \
}                                                              \
void * operator new    (size_t size,const char * classn,const char * file,int line)  THROW_BADALLOC \
{                                                                                                   \
    return tDecoratableManager< CLASS >::Allocate( size, classn, file, line );                      \
}                                                                                                   \
void   operator delete (void * ptr,const char * classn, const char * file,int line)  THROW_NOTHING  \
{                                                                                                   \
    tDecoratableManager< CLASS >::Free( ptr, classn, file, line);                                   \
}                                                                                                   \
private:                                                                                            \
void * operator new[]   ( size_t size ) THROW_BADALLOC;                                             \
void   operator delete[]( void * old ) THROW_NOTHING;                                               \
void * operator new[]   (size_t size,const char *classn,const char * file,int line )  THROW_BADALLOC; \
void   operator delete[](void *ptr,const char *classname,const char * file,int line )  THROW_NOTHING; \
public:

#define DECORATABLE_BASE(CLASS) DECORATABLE(CLASS,int)

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

template< class Decorate >
void * tDecoratableManager< Decorate >::Allocate( size_t size, const char * classn, const char * file, int line )
{
    // delegate to manager
    tDecoratableManagerBase & manager = GetManager();
    return manager.Allocate( size, classn, file, line );
}

// *******************************************************************************
// *
// *	Allocate
// *
// *******************************************************************************
//!
//!		@param	size	size of the memory block to allocate
//!		@return		    the newly allocated block of memory, with space for decorators
//!
// *******************************************************************************

template< class Decorate >
void * tDecoratableManager< Decorate >::Allocate( size_t size )
{
    return Allocate( size, "noclass", "nofile", 0 );
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

template< class Decorate >
void tDecoratableManager< Decorate >::Free( void * ptr, const char * classn, const char * file, int line )
{
    // deregister with manager
    tDecoratableManagerBase & manager = GetManager();
    manager.Free( ptr, classn, file, line );
}

// *******************************************************************************
// *
// *	Free
// *
// *******************************************************************************
//!
//!		@param	ptr	    pointer to the memory area to be freed
//!
// *******************************************************************************

template< class Decorate >
void tDecoratableManager< Decorate >::Free( void * ptr )
{
    return Free( ptr, "noclass", "nofile", 0 );
}

#endif
