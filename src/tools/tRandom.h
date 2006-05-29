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

#ifndef		TRANDOM_H_INCLUDED
#define		TRANDOM_H_INCLUDED

// self include
#ifndef		TRANDOM_H_INCLUDED
#include	"tRandom.h"
#endif

#include    "defs.h"

//! Random generator
class tRandomizer
{
public:
    tRandomizer  ()                                 ;   //!< default constructor
    virtual ~tRandomizer ()                         ;   //!< destructor

    REAL    Get()                                   ;   //!< returns a random float in the interval [0,1)
    int     Get( int max )                          ;   //!< returns a random integer in the interval [0,max)

    static tRandomizer & GetInstance()              ;   //!< returns the standard randomizer
protected:

    unsigned int randMax_;
private:
    tRandomizer  ( tRandomizer const & other )              ;   //!< copy constructor
    tRandomizer & operator =  ( tRandomizer const & other ) ;   //!< copy operator

    virtual unsigned int GetRawRand()                ;   //!< returns a raw random number
};

//! Reproducible random generator
class tReproducibleRandomizer: public tRandomizer
{
public:
    tReproducibleRandomizer  ()                      ;   //!< default constructor
    ~tReproducibleRandomizer ()                      ;   //!< destructor

    static tReproducibleRandomizer & GetInstance()   ;   //!< returns the standard reproducible randomizer
private:
    virtual unsigned int GetRawRand()                ;   //!< returns a raw random number

    int z_, w_;
};

#endif // TRANDOM_H_INCLUDED
