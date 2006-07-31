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

#ifndef ArmageTron_OBSERVER_H
#define ArmageTron_OBSERVER_H

#include "tSafePTR.h"

class nNetObject;

// netobject observer
class nObserver: public tReferencable< nObserver >
{
    friend class tReferencable< nObserver >;
public:
    const nNetObject* GetNetObject() 	const;  // returns the observed object

#ifdef DEBUG
    static void SetLastChecked( const nObserver* checked );
    static bool WasChecked( const nObserver* checked );
#endif

private:
    const nNetObject* object_;   // the observed object

    void SetObject( const nNetObject *);  // sets the observed object

    nObserver();
    ~nObserver();

    friend class nNetObject;
};

// observer pointer
template< class T >
class nObserverPtr
{
public:
    operator bool() const
    {
#ifdef DEBUG
        nObserver::SetLastChecked( this->observer_ );
#endif

        return ( bool( this->observer_ ) && this->observer_->GetNetObject() );
    }

    bool operator!() const
    {
        return !operator bool();
    }

    const T* GetPointer() const
    {
        if ( this->observer_ )
            return dynamic_cast< const T* >( observer_->GetNetObject() );
        else
            return NULL;
    }

    operator const T* () const
    {
        return this->GetPointer();
    }

    const T* operator->() const
    {
        const T* t = *this;

#ifdef DEBUG
        //		tASSERT( nObserver::WasChecked( this->observer_ ) );
#endif

        tASSERT(t);

        return t;
    }

    const T* operator*() const
    {
        const T* t = *this;

#ifdef DEBUG
        //		tASSERT( nObserver::WasChecked( this->observer_ ) );
#endif

        tASSERT(t);

        return t;
    }

    void SetObject ( const T* t)
    {
        if ( t == NULL )
            observer_ = NULL;
        else
            observer_ = &t->GetObserver();
    }

    nObserverPtr& operator = ( const T* t)
    {
        SetObject( t );

        return *this;
    }

    nObserverPtr( const T* t)
    {
        SetObject( t );
    }

    nObserverPtr()
    {
    }
private:
    tCONTROLLED_PTR( nObserver ) observer_;
};

#endif
