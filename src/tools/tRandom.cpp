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

// declaration
#ifndef		TRANDOM_H_INCLUDED
#include	"tRandom.h"
#endif

#ifdef      HAVE_STDLIB
#include    <stdlib.h>
#endif

#include    "tError.h"
#include    "tRecorder.h"
#include    <cstdlib>

#undef 	INLINE_DEF
#define INLINE_DEF

// *******************************************************************************************
// *
// *	tRandomizer
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tRandomizer::tRandomizer( void )
        :randMax_( RAND_MAX )
{
}

// *******************************************************************************************
// *
// *	Get
// *
// *******************************************************************************************
//!
//!		@return		random value between 0 and 1
//!
// *******************************************************************************************

REAL tRandomizer::Get( void )
{
    // Grr. VisualC 6 can't handle this in one line.
    REAL a = GetRawRand();
    REAL b = randMax_;

    return a / b;
}

// *******************************************************************************************
// *
// *	Get
// *
// *******************************************************************************************
//!
//!		@param	max	upper limit of the return value
//!		@return		random integer value between 0 and max - 1
//!
// *******************************************************************************************

int tRandomizer::Get( int max )
{
    int ret = int( floorf( Get() * max ) );
    tASSERT( 0 <= ret );
    tASSERT( ret < max );

    return ret;
}

// *******************************************************************************************
// *
// *	GetInstance
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

tRandomizer & tRandomizer::GetInstance( void )
{
    static tRandomizer randomizer;
    return randomizer;
}

// *******************************************************************************************
// *
// *	~tRandomizer
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tRandomizer::~tRandomizer( void )
{
}

// *******************************************************************************************
// *
// *	tRandomizer
// *
// *******************************************************************************************
//!
//!		@param	other	copy source
//!
// *******************************************************************************************

tRandomizer::tRandomizer( tRandomizer const & other )
{
    tASSERT( 0 ); // not implemented
}

// *******************************************************************************************
// *
// *	operator =
// *
// *******************************************************************************************
//!
//!		@param	other	copy source
//!		@return		    reference to this for chaining
//!
// *******************************************************************************************

tRandomizer & tRandomizer::operator =( tRandomizer const & other )
{
    tASSERT( 0 ); // not implemented

    return *this;
}

// *******************************************************************************************
// *
// *	GetRawRand
// *
// *******************************************************************************************
//!
//!		@return		random value between 0 and randMax_
//!
// *******************************************************************************************

unsigned int tRandomizer::GetRawRand( void )
{
    return rand();
}

static char const * recordingSection = "RANDOM";

// *******************************************************************************************
// *
// *   tReproducibleRandomizer
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tReproducibleRandomizer::tReproducibleRandomizer( void )
{
    // generate random seeds with standard random generator
    z_ = rand();
    w_ = rand();
    randMax_ = 0xffffffff;

    // archive seed
    tRecorder::PlaybackStrict( recordingSection, z_ );
    tRecorder::Record( recordingSection, z_ );

    tRecorder::PlaybackStrict( recordingSection, w_ );
    tRecorder::Record( recordingSection, w_ );

    /*
        // read RAND_MAX from recording or write it to recording
        if ( !tRecorder::PlaybackStrict( recordingSection, randMax_ ) )
            tRecorder::Record( recordingSection, randMax_ );
    */
}

// *******************************************************************************************
// *
// *	~tReproducibleRandomizer
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tReproducibleRandomizer::~tReproducibleRandomizer( void )
{
}

// *******************************************************************************************
// *
// *	GetInstance
// *
// *******************************************************************************************
//!
//!		@return		the default reproducible randomizer
//!
// *******************************************************************************************

tReproducibleRandomizer & tReproducibleRandomizer::GetInstance( void )
{
    static tReproducibleRandomizer randomizer;
    return randomizer;
}

// *******************************************************************************************
// *
// *	GetRawRand
// *
// *******************************************************************************************
//!
//!		@return		raw random value between 0 and randMax_
//!		todo use own, fixed random algorithm so the random values don't have to be stored
// *******************************************************************************************

unsigned int tReproducibleRandomizer::GetRawRand( void )
{
    // advance pseudo random numbers
    z_ = ( 36969 * ( z_ & 0xffff ) + ( z_ >> 16 ) ) & 0xffffffff;
    w_ = ( 18000 * ( w_ & 0xffff ) + ( w_ >> 16 ) ) & 0xffffffff;

    return ( (z_ << 16) + w_ ) & 0xffffffff;

    /*
        unsigned int r = 0;

        // read random value from recording
        if ( !tRecorder::PlaybackStrict( recordingSection, r ) )
        {
            // not found. generate random value
            r = rand();

            // store it
            tRecorder::Record( recordingSection, r );
        }

        return r;
        */
}

